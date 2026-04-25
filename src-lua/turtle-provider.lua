stackSize = 64
logHandle = nil
items = {} --item name (or zero for free slots) -> { chest name, slot, amount }
itemCount = { 0 }

function writeLog(msg)
    logHandle.writeLine(msg)
    print(msg)
end

function setupComms()
    for _, side in pairs(rs.getSides()) do
        if peripheral.isPresent(side) and peripheral.getType(side) == "modem" then
            if not peripheral.wrap(side).isWireless() then
                rednet.open(side)
                return true
            end
        end
    end

    return false
end

function findTurtle(id)
    for _, wt in pairs({ peripheral.find("turtle") }) do
        if wt.getID() == id then
            return peripheral.getName(wt)
        end
    end

    return false
end

function refreshInventory()
    items = {}
    itemCount = { 0 }

    local chests = { peripheral.find("minecraft:chest") }
    for _, inv in pairs(chests) do
        local name = peripheral.getName(inv)
        local invList = inv.list()
        for slot, info in pairs(invList) do
            local itemName = info["name"]
            if not items[itemName] then items[itemName] = {} end
            table.insert(items[itemName], { inv = name, slot = slot, count = info["count"] })
            if not itemCount[itemName] then itemCount[itemName] = 0 end
            itemCount[itemName] = itemCount[itemName] + info["count"]
        end

        for slot = 1, inv.size() do
            if not invList[slot] then
                items[1] = { inv = name, slot = slot }
                itemCount[1] = itemCount[1] + stackSize
            end
        end
    end

    writeLog(string.format("Inventory refreshed, found %d chests.", #chests))
    for name, count in pairs(itemCount) do
        writeLog(string.format("'%s' - %d", name, count))
    end
end

function processRequest(tname, request)
    local itemName = request["item"]
    local amount = request["amount"]
    if not itemCount[itemName] then itemCount[itemName] = 0 end
    local available = itemCount[itemName]

    if not available then itemCount[itemName] = 0 end

    if available < amount then
        if not request["stale"] then writeLog(string.format("Insufficient amount of '%s', %d available but %d was requested.", name, available, amount)) end
        return false
    end

    itemCount[itemName] = available - amount
    itemSlots = items[itemName]
    local required = amount
    for i, info in pairs(itemSlots) do
        local inv = peripheral.wrap(info["inv"])
        local pulled = inv.pushItems(tname, info["slot"], required, request["slot"])
        required = required - pulled
        info["count"] = info["count"] - pulled
        if not info["count"] then
            table.insert(items[1], { inv = info["inv"], slot = info["slot"] })
            table.remove(itemSlots, i)
            i = i - 1
        end
    end

    return true
end

function processUnload(tname, request)
    local itemName = request["item"]
    local amount = request["amount"]
    local slot = request["slot"]
    local freeInSlots = stackSize - math.fmod(itemCount[itemName], stackSize)
    local available = itemCount[1] + freeInSlots --assuming perfectly efficient storage

    if available < amount then
        if not request["stale"] then writeLog(string.format("Not enough space for '%s', %d available but %d was requested.", name, available, amount)) end
        return false
    end

    if amount - freeInSlots > 0 then itemCount[1] = available - stackSize end
    itemSlots = items[itemName]
    local required = amount
    for _, info in pairs(itemSlots) do
        local inv = peripheral.wrap(info["inv"])
        local pulled = inv.pullItems(tname, slot, required, info["slot"])
        required = required - pulled
        info["count"] = info["count"] + pulled
        if required <= 0 then return true end
    end

    local freeSlot = items[1][1]
    local inv = wrap(freeSlot["inv"])
    local pushed = inv.pullItems(tname, slot, required, freeSlot["slot"])
    table.remove(items[1], 0)
    table.insert(items[itemName], { inv = freeSlot["inv"], slot = freeSlot["slot"], count = pushed })
    return true
end

function processEvents()
	local event, arg1, arg2 = os.pullEvent()
    if event == "rednet_message" then
        wrappedTurtle = findTurtle(arg1)
        if not wrappedTurtle then
            writeLog(string.format("Failed to find turtle with ID %d", arg1))
            return false
        end

        if arg2["type"] == "request" then
            if not processRequest(wrappedTurtle, arg2) then
                if not arg2["stale"] then
                    arg2["stale"] = true
                    writeLog("Cannot process request, marking as stale.")
                end
                os.queueEvent("rednet_message", arg1, arg2)
            else
                rednet.send(arg1, { type = "request_done" })
            end
        elseif arg2["type"] == "unload" then
            if not processUnload(wrappedTurtle, arg2) then
                if not arg2["stale"] then
                    arg2["stale"] = true
                    writeLog("Cannot process unload request, marking as stale.")
                end
                os.queueEvent("rednet_message", arg1, arg2)
            else
                rednet.send(arg1, { type = "request_done" })
            end
        elseif arg2["type"] == "refresh_inventory" then
            writeLog("Refreshing inventory...")
            refreshInventory()
        end
    elseif event == "key" then
        if arg1 == keys.enter then refreshInventory() end
    end

    return true
end

print("== Turtle provider v0.5 ==")
logHandle = fs.open("provider-log.txt", fs.exists("turtle-log.txt") and "a" or "w")
if setupComms() then
    refreshInventory()
    rednet.broadcast({ type = "provider_online" })
    while true do processEvents() end
else print("Failed to setup rednet, check that modem is installed on this computer.") end
