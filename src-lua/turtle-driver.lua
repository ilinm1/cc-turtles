processEventTimeout = 0.25

controllerId = nil
logHandle = nil
instructionHandle = nil

running = true
paused = false

action = 0
instructionIndex = 0
materials = nil
refillModemConnected = false

--global coordinates
currentRotation = 1 --1 north, 2 east, 3 south, 4 west
currentX = 0
currentY = 0
currentZ = 0

--misc
function readNext()
    instructionIndex = instructionIndex + 1
    return instructionHandle.read()
end

function findRefillModem()
    for _, side in pairs(rs.getSides()) do
        if peripheral.isPresent(side) and peripheral.getType(side) == "modem" then
            modem = peripheral.wrap(side)
            if not modem.isWireless() then
                rednet.open(side)
                refillModemConnected = true
                return true
            end
        end
    end

    return false
end

function request(id, amount)
    if not refillModemConnected and not findRefillModem() then
        return false, "No wired modem found at refill."
    end

    paused = true
    writeLog(string.format("Requesting %d (material %d) units of '%s'.", amount, id, materials[id]))
    rednet.broadcast({type = "request", item = materials[id], amount = amount, slot = turtle.getSelectedSlot()})
end

function unload(amount)
    if not refillModemConnected and not findRefillModem() then
        return false, "No wired modem found at refill."
    end

    paused = true
    writeLog(string.format("Unloading %d items."), amount)
    rednet.broadcast({type = "unload", amount = amount, slot = turtle.getSelectedSlot()})
end

function writeLog(msg)
    logHandle.writeLine(msg)
    if controllerId ~= nil then rednet.send(controllerId, {type = "msg", data = msg}) end
end

--updates global coordinates using local ones (rotated by turtle's current rotation)
function modifyPosition(x, y, z)
    gx = x
    gy = y
    gz = z

    if currentRotation == 2 then
        gx = y
        gy = -x
    elseif currentRotation == 3 then
        gx = -x
        gy = -y
    elseif currentRotation == 4 then
        gx = -y
        gy = x
    end

    currentX = currentX + gx
    currentY = currentY + gy
    currentZ = currentZ + gz
end

function modifyRotation(x)
    currentRotation = math.fmod(currentRotation + x, 4)
end

--using global coordinates
function moveBy(x, y, z)
    while currentRotation ~= 1 do
        turtle.turnRight()
        modifyRotation(1)
    end

    for _ = 1, y do
        turtle.forward()
    end

    if x > 0 then turtle.turnRight()
    elseif x < 0 then turtle.turnLeft() end

    for _ = 1, x do
        turtle.forward()
    end

    for _ = 1, z do
        if z > 0 then turtle.up() else turtle.down() end
    end

    modifyPosition(x, y, z)
end

function checkAction(result, err)
    if not result then
        msg = string.format("Action has failed: %s - %s - '%s'.", instructionIndex, action, err)
        writeLog(msg)
    end
end

--setup

function readMaterialData()
    str = ""
    strEnd = false
    while true do
        ch = readNext()

        if not ch then
            if strEnd then --two zeroes in a row -> end of material data
                break
            end
            materials[#materials] = str
            strEnd = true
        else
            str .. ch
            strEnd = false
        end
    end

    writeLog(string.format("Material data read, %d materials registered.", #materials))
    for id = 0, #materials - 1 do
        writeLog(string.format("Material %d - '%s'.", id, materials[id]))
    end
end

function setupWirelessComm()
    for _, side in pairs(rs.getSides()) do
        if peripheral.isPresent(side) and peripheral.getType(side) == "modem" then
            if peripheral.wrap(side).isWireless() then
                rednet.open(side)
                return true
            end
        end
    end

    return false
end

--main loop

function nextAction()
    if waitingForProvider then return false end

    action = readNext()

    if action == nil then
        writeLog("EOF in instruction file.")
        return false
    end

    if          action == 1     then    checkAction(turtle.forward())    modifyPosition(0, 1, 0)
    elseif      action == 2     then    checkAction(turtle.back())       modifyPosition(0, -1, 0)
    elseif      action == 3     then    checkAction(turtle.up())         modifyPosition(0, 0, 1)
    elseif      action == 4     then    checkAction(turtle.down())       modifyPosition(0, 0, -1)
    elseif      action == 5     then    checkAction(turtle.turnLeft())   modifyRotation(3)
    elseif      action == 6     then    checkAction(turtle.turnRight())  modifyRotation(1)
    elseif      action == 7     then    checkAction(turtle.dig())
    elseif      action == 8     then    checkAction(turtle.digUp())
    elseif      action == 9     then    checkAction(turtle.digDown())
    elseif      action == 10    then    checkAction(turtle.place())
    elseif      action == 11    then    checkAction(turtle.placeUp())
    elseif      action == 12    then    checkAction(turtle.placeDown())
    elseif      action == 13    then    checkAction(turtle.select(readNext()))
    elseif      action == 14    then    checkAction(request(readNext(), readNext()))
    elseif      action == 15    then    checkAction(unload(readNext()))
    elseif      action == 16    then    checkAction(turtle.refuel(readNext()))
    else writeLog("Unknown action/NOP")
    end

    return true
end

function processMessage(id, msg)
    if id ~= controllerId then return end

    if msg["type"] == "status" then
        writeLog("Sending status.")
        rednet.send(id,
            {
                type = "status",
                data =
                {
                    instr = instructionIndex,
                    x = currentX,
                    y = currentY,
                    z = currentZ,
                    rot = currentRotation,
                    fuel = turtle.getFuelLevel(),
                    slot = turtle.getSelectedSlot(),
                    paused = paused,
                    onRefill = refillModemConnected
                }
            })
    elseif msg["type"] == "pause" then
        writeLog("Pause toggled.")
        return true, true
    elseif msg["type"] == "stop" then
        writeLog("Stop request received.")
        return false, false
    elseif msg["type"] == "jump" then
        writeLog(string.format("Jumping to instruction no. %d.", msg["instr"]))
        instructionIndex = msg["instr"]
        instructionHandle.seek("set", instructionIndex)
    elseif msg["type"] == "request_done" then
        paused = false
    end

    return true, false
end

function processEvents()
    local timerId = nil
    if not paused then
        timerId = os.startTimer(processEventTimeout)
    end

    while true do
        local event, arg1, arg2 = os.pullEvent()

        if event == "rednet_message" then
            return processMessage(arg1, arg2)
        end

        if event == "timer" and arg1 == timerId then
            break
        end
    end

    return true, false
end

function nextAction_wrapper()
    if not paused then
        local result, arg1 = pcall(nextAction)
        if not result then
            writeLog("Unexpected error while performing action: "..tostring(arg1).."; Continuing.")
        else
            if not arg1 then
                writeLog("nextAction() has returned false, exiting loop.")
                running = false
            end
        end
    end
end

function processEvents_wrapper()
    local result, arg1, arg2 = pcall(processEvents)
    if not result then
        writeLog("Unexpected error while processing events: "..tostring(running).."; Continuing.")
    else
        if not arg1 then
            writeLog("processEvents() has returned false, exiting loop.")
            running = false
        end

        if arg2 then paused = not paused end
    end
end

--entrypoint

print("== Turtle driver v0.5 ==")
logHandle = fs.open("turtle-log.txt", fs.exists("turtle-log.txt") and "a" or "w")

print("Instruction file path: ")
instructionHandle = fs.open(read(), "rb")

print("Offset X Y Z: ")
offsetStr = read()
if offsetStr ~= "" then
    offsetToks = {}
    for t in string.gmatch(offsetStr, "[^%s]+") do offsetToks[#offsetToks+1] = t end

    offsetX = tonumber(offsetToks[1])
    offsetY = tonumber(offsetToks[2])
    offsetZ = tonumber(offsetToks[3])
end

print("Instruction offset: ")
instrOffsetStr = read()
if instrOffsetStr ~= "" then
    instructionHandle.seek("set", tonumber(instrOffsetStr))
end

print("Controller ID: ")
controllerIdStr = read()
if controllerIdStr ~= "" then
    controllerId = tonumber(controllerIdStr)
end

if controllerId == nil or setupWirelessComm() then
    print("Started. Use turtle controller to send commands and see errors.")

    moveBy(offsetX, offsetY, offsetZ)
    readMaterialData()

    while running do parallel.waitForAll(processEvents_wrapper, nextAction_wrapper) end

    writeLog("Finishing.")
    logHandle.close()
    instructionHandle.close()
else print("Failed to setup rednet, make sure that you're using turtle with a modem (or if you don't need remote control leave controller ID empty).") end
