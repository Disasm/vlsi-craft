local args = {...}

function usage()
    print("Usage: gcode-server {params}")
    print("    shell <channel>")
    print("    file <channel> <path>")
    print("    pastebin <channel> <paste_id>")
end

if #args < 1 then
    usage()
    return
end

local cmd = args[1]
local channel = nil
local pasteId = nil
local filePath = nil

if cmd == "shell" then
    if #args < 2 then
        usage()
        return
    end
    channel = tonumber(args[2])
elseif cmd == "file" then
    if #args < 3 then
        usage()
        return
    end
    channel = tonumber(args[2])
    filePath = args[3]
elseif cmd == "pastebin" then
    if #args < 3 then
        usage()
        return
    end
    channel = tonumber(args[2])
    pasteId = args[3]
else
    print("Unknown command")
    return
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

local serverChannel = math.random(1,65535)
print("ServerChannel: "..serverChannel..", ClientChannel: "..channel)

modem.open(serverChannel)
print("Listening channel "..serverChannel)

local terminate = false

function readFunction()
    local s = read()
    if s == "q" or s == "quit" then
        terminate = true
        return
    end
    modem.transmit(channel, serverChannel, string.upper(s))
end

function recvFunction()
    local event, p1, p2, p3, message = os.pullEvent("modem_message")
    if event == "modem_message" then
        print(message)
    end
end

if cmd == "shell" then
    while not terminate do
        parallel.waitForAny(readFunction, recvFunction)
    end
elseif cmd == "file" or cmd == "pastebin" then
    if cmd == "pastebin" then
        filePath = "pastebin.gcode"
        if fs.exists(filePath) then
            fs.delete(filePath)
        end
        shell.run("pastebin", "get "..pasteId.." "..filePath)
    end

    local f = fs.open(filePath, 'r')
    if f == nil then
        print("Can't open script file "..filePath)
        return
    end

    print("Execution started. Press A to abort.")
    sleep(1)

    local lines = {}
    local currentLine = 0
    while true do
        local line = f.readLine()
        if line == nil then
            break
        end
        if string.len(line) > 0 then
            --print("line='"..line.."'")
            currentLine = currentLine + 1
            lines[currentLine] = line
        end
    end

    currentLine = 1
    while true do
        if currentLine > #lines then
            break
        end
        local command = lines[currentLine]

        command = string.upper(command)
        modem.transmit(channel, serverChannel, command)
        print(command)

        local abort = false
        local reply = nil

        while true do
            local event, p1, p2, p3, p4 = os.pullEvent()
            if event == "modem_message" then
                reply = p4
                print(reply)
                break
            elseif event == "key" then
                local keycode = p1
                if keycode == keys.a then
                    print("Execution aborted")
                    abort = true
                    break
                end
            end
        end

        if abort then
            break
        end

        local replyCode = nil

        local p = string.find(reply, " ")
        if p ~= nil then
            replyCode = string.sub(reply, 1, p - 1)
        else
            replyCode = reply
        end

        if replyCode == "error" then
            print("Error occured. Following actions are possible")
            print("    A - abort")
            print("    R - restart from the beginning")
            print("    C - continue from the current command")
            print("    N - continue from the next command")

            local action = nil

            while true do
                local event, keycode = os.pullEvent("key")
                if keycode == keys.a then
                    action = "abort"
                    break
                elseif keycode == keys.r then
                    action = "restart"
                    break
                elseif keycode == keys.c then
                    action = "continueFromCurrent"
                    break
                elseif keycode == keys.n then
                    action = "continueFromNext"
                    break
                end
            end

            if action == "abort" then
                print("Execution aborted")
                break
            elseif action == "restart" then
                print("Execution will restart from the beginning")
                currentLine = 1
            elseif action == "continueFromCurrent" then
                print("Execution will continue from the current command")
                currentLine = currentLine + 0
            elseif action == "continueFromNext" then
                print("Execution will continue from the next command")
                currentLine = currentLine + 1
            end
        else
            currentLine = currentLine + 1
        end
    end
end
