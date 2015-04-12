--[[
Command parameters
------------------
X, Y, Z - coordinates
R - direction
S - slot number
C - count, amount

Commands
--------
G0: Move
G0 Xnnn Ynnn Znnn Rnnn

G4: Dwell
Examples:
G4 P200 (200 ms)
G4 S2 (2 s)

G28: Move to Origin (Home)
Examples:
G28 (Go to origin on all axes)
G28 X Z (Go to origin only on the X and Z axis)

G90: Set to Absolute Positioning
G91: Set to Relative Positioning

G92: Set Position
Example: G92 X10 R3



M50: Place Forward
M50 Snnn

M51: Place Down
M51 Snnn

M52: Place Up
M52 Snnn

M53: Dig Forward
M54: Dig Down
M55: Dig Up

M56: Suck Forward
M56 Cnnn

M57: Suck Down
M57 Cnnn

M58: Suck Up
M58 Cnnn

M60: Get Position
M60
ok X:nnn Y:nnn Z:nnn R:nnn

M61: Get Fuel Level
M61
ok C:nnn (-1 in case of disabled fuel)

M62: Get Fuel Limit
M62
ok C:nnn (-1 in case of unlimited fuel)

M63: Refuel
M63 Snnn [Cnnn]

M64: Get Item Count
M64 S1
ok C:64

M65: Select Slot
M65 Snnn

M66: Gather
M66 [Snnn]
]]--

state = {}
state.x = 0;
state.y = 0;
state.z = 0;
state.r = 0;
state.absolutePositioning = true;
state.channel = 0;

function saveState()
    local file = fs.open("state.txt", "w")
    file.writeLine(textutils.serialize(state))
    file.close()
end

function loadState()
    if fs.exists("state.txt") then
        local file = fs.open("state.txt", "r")
        state1 = textutils.unserialize(file.readAll())

        -- merge state
        for k,v in pairs(state1) do
            state[k] = v
        end

        file.close()
        return true
    else
        return false
    end
end

function parseCode(s)
    local c = string.sub(s, 1, 1)
    local p = string.sub(s, 2)
    return c, tonumber(p)
end

function parseCodes(s)
    local codes = {}
    if string.len(s) == 0 then
        return codes
    end
    local index = 1
    s = s.." "
    while index < string.len(s) do
        local p = string.find(s, " ", index, true)
        if p == nil then
            --print("p is nil")
            break
        end
        --print("index=="..index..", p=="..p)
        local sub = string.sub(s, index, p - 1)
        if string.len(sub) == 0 then
            --print("sub.len() == 0")
            break
        end

        --print("sub=='"..sub.."', s.len()=="..string.len(s)..", index=="..index..", p=="..p)
        index = p + 1

        local code = {}
        code.code, code.value = parseCode(sub)
        codes[#codes + 1] = code
    end
    return codes
end

function gatherSlot(toSlot, startFromSlot)
    if turtle.getItemSpace(toSlot) == 0 or turtle.getItemCount(toSlot) == 0 then
        return
    end
    local i
    for i = startFromSlot,16 do
        if i ~= toSlot then
            turtle.select(i)
            if turtle.compareTo(toSlot) then
                turtle.transferTo(toSlot)
            end
        end
    end
end

function gatherAll()
    local i
    for i = 1,15 do
        gatherSlot(i, i + 1)
    end
end

local directionTable = {}
directionTable[0] = {dx =  0, dz =  1}
directionTable[1] = {dx =  1, dz =  0}
directionTable[2] = {dx =  0, dz = -1}
directionTable[3] = {dx = -1, dz =  0}

function getDirection(dx, dz)
    for i = 0,3 do
        local dir = directionTable[i]
        if dir.dx == dx and dir.dz == dz then
            return i
        end
    end
    return -1
end

function sign(v)
    if v > 0 then
        return 1
    elseif v < 0 then
        return -1
    else
        return 0
    end
end

function turnLeft()
    turtle.turnLeft()
    state.r = (state.r + 1) % 4
end

function turnRight()
    turtle.turnRight()
    state.r = (state.r - 1 + 4) % 4
end

function turn(r)
    local delta = r - state.r
    if delta < 0 then delta = delta + 4 end

    local deltaRight = delta
    local deltaLeft = 4 - deltaRight

    if deltaRight < deltaLeft then
        while r ~= state.r do
            turnLeft()
        end
    else
        while r ~= state.r do
            turnRight()
        end
    end
end

function forceForward()
    local dir = directionTable[state.r]

    local tries = 0
    while true do
        tries = tries + 1
        if tries > 10 then
            return false
        end

        if turtle.forward() then
            state.x = state.x + dir.dx;
            state.z = state.z + dir.dz;
            saveState()
            return true
        else
            turtle.dig()
            sleep(0.5)
        end
    end
end

function forceUp()
    local tries = 0
    while true do
        tries = tries + 1
        if tries > 5 then
            return false
        end

        if turtle.up() then
            state.y = state.y + 1;
            saveState()
            return true
        else
            turtle.digUp()
            sleep(0.5)
        end
    end
end

function forceDown()
    local tries = 0
    while true do
        tries = tries + 1
        if tries > 5 then
            return false
        end

        if turtle.down() then
            state.y = state.y - 1;
            saveState()
            return true
        else
            turtle.digDown()
            sleep(0.5)
        end
    end
end

function goto(x, y, z, r)
    if state.x ~= x then
        local targetDx = sign(x - state.x)
        local r1 = getDirection(targetDx, 0)
        turn(r1)
        saveState()
        while state.x ~= x do
            if forceForward() == false then
                return false
            end
        end
    end

    if state.z ~= z then
        local targetDz = sign(z - state.z)
        local r1 = getDirection(0, targetDz)
        turn(r1)
        saveState()
        while state.z ~= z do
            if forceForward() == false then
                return false
            end
        end
    end

    if state.y ~= y then
        if y > state.y then
            while y ~= state.y do
                forceUp()
            end
        else
            while y ~= state.y do
                forceDown()
            end
        end
    end

    if state.r ~= r then
        turn(r)
        saveState()
    end

    return true
end

if loadState() == false then
    state.channel = math.random(1,65535)
    os.setComputerLabel("gturtle"..state.channel)
    saveState()
end

local modem = nil
for t=1,5 do
    Sides = peripheral.getNames()
    for i = 1,#Sides do
        if peripheral.getType(Sides[i]) == "modem" then
            modem = peripheral.wrap(Sides[i])
            break
        end
    end
    if modem ~= nil then
        break
    else
        sleep(0.5)
    end
end

if modem == nil then
    print("Can't find modem. Press any key to shutdown.")
    os.pullEvent("key")
    os.shutdown()
end

modem.open(state.channel)
print("Listening channel "..state.channel)
local replyChannel = nil

function reply(msg)
    modem.transmit(replyChannel, state.channel, msg)
end

function processCommand(cmd, params)
    if cmd.value ~= nil then
        print("Command: "..cmd.code..cmd.value)
    end

    if cmd.code == 'G' then
        if cmd.value == 0 or cmd.value == 1 then
            -- Move
            local tx = state.x
            local ty = state.y
            local tz = state.z
            local tr = state.r
            local err = false
            for i = 1,#params do
                local arg = params[i]
                if arg.value == nil then
                    err = true
                    break
                end
                if arg.code == 'X' then
                    if state.absolutePositioning then
                        tx = arg.value
                    else
                        tx = arg.value + state.x
                    end
                elseif arg.code == 'Y' then
                    if state.absolutePositioning then
                        ty = arg.value
                    else
                        ty = arg.value + state.y
                    end
                elseif arg.code == 'Z' then
                    if state.absolutePositioning then
                        tz = arg.value
                    else
                        tz = arg.value + state.z
                    end
                elseif arg.code == 'R' then
                    if state.absolutePositioning then
                        tr = arg.value
                    else
                        tr = (arg.value + state.r) % 4
                    end
                else
                    err = true
                end
            end

            if err or #params == 0 then
                reply("error Invalid parameters")
            else
                if goto(tx, ty, tz, tr) then
                    reply("ok")
                else
                    reply("error Movement error")
                end
            end

        elseif cmd.value == 4 then
            -- Dwell
            local seconds = 0
            if #params > 0 then
                if params[1].code == 'P' then
                    seconds = params[1].value / 1000.0
                end
                if params[1].code == 'S' then
                    seconds = params[1].value
                end
            end
            if seconds > 0 then
                os.sleep(seconds)
                reply("ok")
            else
                reply("error Invalid parameters")
            end

        elseif cmd.value == 28 then
            -- Move to Origin (Home)
            local tx
            local ty
            local tz
            local tr
            local err = false

            if #params == 0 then
                tx = 0
                ty = 0
                tz = 0
                tr = 0
            else
                tx = state.x
                ty = state.y
                tz = state.z
                tr = state.r
                for i = 1,#params do
                    local arg = params[i]
                    if arg.code == 'X' then
                        tx = 0
                    elseif arg.code == 'Y' then
                        ty = 0
                    elseif arg.code == 'Z' then
                        tz = 0
                    elseif arg.code == 'R' then
                        tr = 0
                    else
                        err = true
                    end
                end
            end

            if err then
                reply("error Invalid parameters")
            else
                if goto(tx, ty, tz, tr) then
                    reply("ok")
                else
                    reply("error Movement error")
                end
            end

        elseif cmd.value == 90 then
            -- Set to Absolute Positioning
            state.absolutePositioning = true
            reply("ok")

        elseif cmd.value == 91 then
            -- Set to Relative Positioning
            state.absolutePositioning = false
            reply("ok")

        elseif cmd.value == 92 then
            -- Set Position
            local tx = state.x
            local ty = state.y
            local tz = state.z
            local tr = state.r
            local err = false
            for i = 1,#params do
                local arg = params[i]
                if arg.value == nil then
                    err = true
                    break
                end
                if arg.code == 'X' then
                    tx = arg.value
                elseif arg.code == 'Y' then
                    ty = arg.value
                elseif arg.code == 'Z' then
                    tz = arg.value
                elseif arg.code == 'R' then
                    tr = arg.value
                else
                    err = true
                end
            end

            if err or #params == 0 then
                reply("error Invalid parameters")
            else
                state.x = tx
                state.y = ty
                state.z = tz
                state.r = tr
                reply("ok")
            end

        else
            reply("error Invalid command")
        end

    elseif cmd.code == 'M' then
        if cmd.value == 50 or cmd.value == 51 or cmd.value == 52 then
            -- Place Forward, Place Down, Place Up
            local slot = -1
            if #params >= 1 then
                if params[1].code == 'S' and params[1].value ~= nil then
                    slot = params[1].value
                end
            end

            if slot >= 1 and slot <= 16 then
                turtle.select(slot)
                local ok = false
                if cmd.value == 50 then
                    ok = turtle.place()
                elseif cmd.value == 51 then
                    ok = turtle.placeDown()
                elseif cmd.value == 52 then
                    ok = turtle.placeUp()
                end

                if ok then
                    reply("ok")
                else
                    reply("error Can't place")
                end
            else
                reply("error Invalid parameters")
            end

        elseif cmd.value == 53 or cmd.value == 54 or cmd.value == 55 then
            -- Dig Forward, Dig Down, Dig Up
            local ok = false
            local i
            for i = 1,10 do
                if cmd.value == 53 then
                    ok = turtle.dig()
                elseif cmd.value == 54 then
                    ok = turtle.digDown()
                elseif cmd.value == 55 then
                    ok = turtle.digUp()
                end
                if ok == false then
                    os.sleep(0.5)
                else
                    break
                end
            end

            if ok then
                reply("ok")
            else
                reply("error Can't dig")
            end

        elseif cmd.value == 56 or cmd.value == 57 or cmd.value == 58 then
            -- Suck Forward, Suck Down, Suck Up
            local amount = -1
            if #params >= 1 then
                if params[1].code == 'C' and params[1].value ~= nil then
                    amount = params[1].value
                end
            end

            if amount > 0 then
                local ok = false

                if cmd.value == 56 then
                    ok = turtle.suck(amount)
                elseif cmd.value == 57 then
                    ok = turtle.suckDown(amount)
                elseif cmd.value == 58 then
                    ok = turtle.suckUp(amount)
                end

                if ok then
                    reply("ok")
                else
                    reply("error Can't suck")
                end
            else
                reply("error Invalid parameters")
            end

        elseif cmd.value == 60 then
            -- Get Position
            reply("ok X:"..state.x.." Y:"..state.y.." Z:"..state.z.." R:"..state.r)

        elseif cmd.value == 61 then
            -- Get Fuel Level
            local level = turtle.getFuelLevel()
            if level == "unlimited" then
                reply("ok C:-1")
            else
                reply("ok C:"..level)
            end

        elseif cmd.value == 62 then
            -- Get Fuel Limit
            local limit = turtle.getFuelLimit()
            if limit == "unlimited" then
                reply("ok C:-1")
            else
                reply("ok C:"..limit)
            end

        elseif cmd.value == 63 then
            -- Refuel
            local slot = -1
            local amount = 1
            if #params >= 1 then
                if params[1].code == 'S' and params[1].value ~= nil then
                    slot = params[1].value
                end

                if #params >= 2 then
                    if params[2].code == 'C' and params[2].value ~= nil then
                        amount = params[2].value
                    end
                end
            end

            if slot >= 1 and slot <= 16 and amount > 0 then
                turtle.select(slot)
                if turtle.refuel(amount) then
                    reply("ok")
                else
                    reply("error Can't refuel")
                end
            else
                reply("error Invalid parameters")
            end

        elseif cmd.value == 64 then
            -- Get Item Count
            local slot = -1
            if #params >= 1 then
                if params[1].code == 'S' and params[1].value ~= nil then
                    slot = params[1].value
                end
            end

            if slot >= 1 and slot <= 16 then
                reply("ok C:"..turtle.getItemCount(slot))
            else
                reply("error Invalid parameters")
            end

        elseif cmd.value == 65 then
            -- Select Slot
            local slot = -1
            if #params >= 1 then
                if params[1].code == 'S' and params[1].value ~= nil then
                    slot = params[1].value
                end
            end

            if slot >= 1 and slot <= 16 then
                turtle.select(slot)
                reply("ok")
            else
                reply("error Invalid parameters")
            end

        elseif cmd.value == 66 then
            -- Gather
            local slot = nil
            if #params >= 1 then
                if params[1].code == 'S' and params[1].value ~= nil then
                    slot = params[1].value
                end
            end

            if slot == nil then
                gatherAll()
                reply("ok")
            else
                if slot >= 1 and slot <= 16 then
                    gatherSlot(slot, 1)
                    reply("ok")
                else
                    reply("error Invalid parameters")
                end
            end
        else
            reply("error Invalid command")
        end
    else
        reply("error Invalid command")
    end
end

while true do
    local event, p1, p2, p3, p4 = os.pullEvent()
    if event == "modem_message" then
        local message = p4
        replyChannel = p3
        print("Message: "..message..", replyChannel: "..replyChannel)

        local codes = parseCodes(message)
        if #codes > 0 then
            local cmd = codes[1]

            local params = {}
            if #codes >= 2 then
                for i = 2,#codes do
                    params[i-1] = codes[i]
                end
            end

            processCommand(cmd, params)
        end
    end
end
