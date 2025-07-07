commandBuffer = ""
maxX, maxY = 0, 0
selectedTurtle = 0

function setupRednet()
    if rednet.isOpen() then
        return true
    end
    for _, side in ipairs( rs.getSides() ) do
        if peripheral.isPresent(side) and peripheral.getType(side) == "modem" then
            rednet.open(side)
            return true
        end
    end
    return false
end

function writeMessage(message)
    if string.len(message) > maxX then
        writeMessage(string.sub(message, 1, maxX))
        writeMessage(string.sub(message, maxX + 1, string.len(message)))
        return
    end

    local _, y = term.getCursorPos()
    if y == maxY  then
        term.clearLine()
        y = y - 1
        term.scroll(1)
    end
    term.setCursorPos(1, y)
    term.write(message)
    term.setCursorPos(1, y + 1)
end

function writeCommandBuffer()
    local x, y = term.getCursorPos()
    term.setCursorPos(1, maxY)
    term.clearLine()
    term.write(">"..commandBuffer)
    term.setCursorPos(x, y)
end

function processEvents()
    writeCommandBuffer()
    local event, arg1, arg2 = os.pullEvent()
    if event == "rednet_message" then
        if arg2["type"] == "msg" then
            writeMessage(string.format("%d | %s", arg1, arg2["data"]))
        elseif arg2["type"] == "status" then
            writeMessage(string.format(
                "Status for %d | Instruction: %d | X: %d | Y: %d | Z: %d | Rotation: %d | Fuel: %d | Slot: %d",
                arg1,
                arg2["data"]["instr"],
                arg2["data"]["x"],
                arg2["data"]["y"],
                arg2["data"]["z"],
                arg2["data"]["rot"],
                arg2["data"]["fuel"],
                arg2["data"]["slot"]
            ))
        end
    elseif event == "char" then 
        commandBuffer = commandBuffer..arg1
    elseif event == "key" then
        if arg1 == keys.backspace then
            commandBuffer = string.sub(commandBuffer, 1, string.len(commandBuffer) - 1)
        elseif arg1 == keys.enter then
            local commandToks = {}
            for t in string.gmatch(commandBuffer, "[^%s]+") do commandToks[#commandToks+1] = t end

            if string.sub(commandBuffer, 1, 7) == "select " then
                selectedTurtle = tonumber(commandToks[2])
                writeMessage("Turtle selected.")
            elseif string.sub(commandBuffer, 1, 6) == "status" then rednet.send(selectedTurtle, {type = "status"})
            elseif string.sub(commandBuffer, 1, 5) == "pause" then rednet.send(selectedTurtle, {type = "pause"})
            elseif string.sub(commandBuffer, 1, 8) == "stop" then rednet.send(selectedTurtle, {type = "stop"})
            elseif string.sub(commandBuffer, 1, 5) == "jump " then rednet.send(selectedTurtle, {type = "jump", instr = tonumber(commandToks[2])})
            else writeMessage("Unknown command.")
            end

            commandBuffer = ""
        end
    end
end

--Driver code
print("== Turtle controller v0.5 ==")

if(setupRednet()) then
    maxX, maxY = term.getSize()
    while true do processEvents() end
else print("Failed to setup rednet, please check that modem is installed on this computer.") end
