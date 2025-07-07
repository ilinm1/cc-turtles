inventorySize = 16
processEventTimeout = 0.25

controllerId = nil
logHandle = nil
instructionHandle = nil

running = true
paused = false

action = 0
instructionIndex = 0

--global coordinates
currentRotation = 1 --1 north, 2 east, 3 south, 4 west
currentX = 0
currentY = 0
currentZ = 0

remainingMaterials = 0
lastMaterial = 1

fuelSlots = {}
materialSlots = {}

--utilities

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

function selectMaterial(materialId)
    writeLog(string.format("Selecting material %d.", materialId))
    lastMaterial = materialId

    for index, slot in pairs(materialSlots[materialId]) do
        if turtle.getItemCount(slot) == 0 then
            writeLog(string.format("Material slot %d is depleted.", slot))
            materialSlots[materialId][index] = nil
        else
            writeLog(string.format("Selecting slot %d.", slot))
            remainingMaterials = turtle.getItemCount(slot)
            return turtle.select(slot)
        end
    end

    return false, string.format("All material slots for material ID %d are depleted.", materialId)
end

function refuel()
    if #fuelSlots == 0 then
        for slot = 1, inventorySize do
            turtle.select(slot)
            if turtle.refuel(0) then fuelSlots[#fuelSlots+1] = slot end
        end
    end

    refueled = false

    for index, slot in pairs(fuelSlots) do
        writeLog(string.format("Attempting to refuel using slot %d.", slot))
        turtle.select(slot)
        if not turtle.refuel() then
            writeLog(string.format("Fuel slot %d is depleted.", slot))
            fuelSlots[index] = nil
        else
            refueled = true
        end
    end

    if not refueled then
        writeLog("Failed to refuel using any of the fuel slots.")
        return false
    end
end

--setup

function readMaterialData()
    zeroRead = false
    slots = {}

    while true do
        slot = instructionHandle.read()
        instructionIndex = instructionIndex + 1

        if slot == 0 then
            if zeroRead then break end --all materials read
            zeroRead = true
            materialSlots[#materialSlots+1] = slots
            slots = {}
        else
            zeroRead = false
            slots[#slots+1] = slot
        end
    end

    writeLog(string.format("Material data read, %d materials registered.", #materialSlots))
    for materialId = 1, #materialSlots do
        writeLog(string.format("Material %d", materialId))
        for index, slot in pairs(materialSlots[materialId]) do writeLog(string.format("%d - %d", index, slot)) end
    end
end

function setupRednet()
    if rednet.isOpen() then
        return true
    end

    for _, side in pairs(rs.getSides()) do
        if peripheral.isPresent(side) and peripheral.getType(side) == "modem" then
            rednet.open(side)
            return true
        end
    end

    return false
end

--work

function nextAction()
    action = instructionHandle.read()
    instructionIndex = instructionIndex + 1

    if action == nil then
        writeLog("EOF in instruction file.")
        return false
    end

    if turtle.getFuelLevel() == 0 then
        if not refuel() then return false end
    end

    if action > 9 and action < 13 then
        if remainingMaterials <= 0 then checkAction(selectMaterial(lastMaterial)) end
        remainingMaterials = remainingMaterials - 1
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
    elseif      action == 13    then    checkAction(selectMaterial(1))
    elseif      action == 14    then    checkAction(selectMaterial(2))
    elseif      action == 15    then    checkAction(selectMaterial(3))
    elseif      action == 16    then    checkAction(selectMaterial(4))
    elseif      action == 17    then    checkAction(selectMaterial(5))
    elseif      action == 18    then    checkAction(selectMaterial(6))
    elseif      action == 19    then    checkAction(selectMaterial(7))
    elseif      action == 20    then    checkAction(selectMaterial(8))
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
    end

    return true, false
end

function processEvents()
    local timerId = os.startTimer(processEventTimeout)

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

--work wrappers

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

--driver

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

if controllerId == nil or setupRednet() then
    print("Started. Use turtle controller to send commands and see errors.")

    moveBy(offsetX, offsetY, offsetZ)
    readMaterialData()

    while running do parallel.waitForAll(processEvents_wrapper, nextAction_wrapper) end

    writeLog("Finishing.")
    logHandle.close()
    instructionHandle.close()
else print("Failed to setup rednet, make sure that you're using turtle with a modem.") end
