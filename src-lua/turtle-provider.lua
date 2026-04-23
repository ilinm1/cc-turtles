stackSize = 64
logHandle = nil
items = nil --item name (or zero for free slots) -> { chest name, slot, amount }
itemCount = nil

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
    for _, wt in pairs(peripheral.find("turtle")) do
        if wt.getId() == id then
            return wt
        end
    end

    return false
end

function refreshInventory()
    items = nil
    itemCount = nil
    freeSlots = 0

    local chests = peripheral.find("minecraft:chest")
    for _, inv in pairs(chests) do
        local name = peripheral.getName(inv)
        for slot, info in pairs(inv.list()) do
            if info then
                local itemName = info["name"]
                items[itemName][#items[itemName]] = { inv = name, slot = slot, count = info["count"] }
                itemCount[itemName] = itemCount[itemName] + info["count"]
            else
                items[0] = { inv = name, slot = slot }
                itemCount[0] = itemCount[0] + stackSize
            end
        end
    end
    writeLog(string.format("Inventory refreshed, obtained info for %d item types from %d chests.", #items, #chests))
end

function processRequest(wrappedTurtle, request)
    local itemName = request["item"]
    local amount = request["amount"]
    local available = itemCount[itemName]

    if available < amount then
        if not request["stale"] then writeLog(string.format("Insufficient amount of '%s', %d available but %d was requested.", name, available, amount)) end
        return false
    end

    itemCount[itemName] = available - amount
    itemSlots = items[itemName]
    local required = amount
    for i, info in pairs(itemSlots) do
        local pulled = wrappedTurtle.pullItems(info["inv"], info["slot"], required, request["slot"])
        required = required - pulled
        info["count"] = info["count"] - pulled
        if not info["count"] then
            items[0][#items[0]] = { inv = info["inv"], slot = info["slot"] }
            itemSlots.remove(i)
            i = i - 1
        end
    end

    return true
end

function processUnload(wrappedTurtle, request)
    local selectedInfo = wrappedTurtle.getItemDetail()
    local itemName = selectedInfo["name"]
    local amount = math.min(request["amount"], selectedInfo["count"])
    local freeInSlots = stackSize - math.fmod(itemCount[itemName], stackSize)
    local available = itemCount[0] + freeInSlots --assuming perfectly efficient storage

    if available < amount then
        if not request["stale"] then writeLog(string.format("Not enough space for '%s', %d available but %d was requested.", name, available, amount)) end
        return false
    end

    itemCount[0] = available - (request["amount"] - freeInSlots > 0) ? stackSize : 0 
    itemSlots = items[itemName]
    local required = amount
    for _, info in pairs(itemSlots) do
        local pushed = wrappedTurtle.pushItems(info["inv"], request["slot"], required, info["slot"])
        required = required - pushed
        info["count"] = info["count"] + pushed
        if required <= 0 then return true end
    end

    local freeSlot = items[0][0]
    local pushed = wrappedTurtle.pushItems(freeSlot["inv"], request["slot"], required, freeSlot["slot"])
    items[0].remove(0)
    items[itemName][#items[itemName]] = { inv = freeSlot["inv"], slot = freeSlot["slot"], count = pushed }
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
            if not processRequest(wrappedTurtle, request) then
                if not arg2["stale"] then
                    args["stale"] = true
                    writeLog("Cannot process request, marking as stale.")
                end
                os.queueEvent("rednet_message", arg1, arg2)
            end
        elseif arg2["type"] == "unload" then
            if not processUnload(wrappedTurtle, request) then
                if not arg2["stale"] then
                    args["stale"] = true
                    writeLog("Cannot process unload request, marking as stale.")
                end
                os.queueEvent("rednet_message", arg1, arg2)
            end
        elseif arg2["type"] == "refresh_inventory" then
            writeLog("Refreshing inventory...")
            refreshInventory()
        end
    end

    return true
end

print("== Turtle provider v0.1 ==")
logHandle = fs.open("provider-log.txt", fs.exists("turtle-log.txt") and "a" or "w")
if setupComms() then
    refreshInventory()
    while true do processEvents() end
else print("Failed to setup rednet, check that modem is installed on this computer.") end
