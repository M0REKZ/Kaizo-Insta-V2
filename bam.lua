Import("configure.lua")

--- Setup Config -------
config = NewConfig()
config:Add(OptCCompiler("compiler"))
config:Add(OptLibrary("zlib", "zlib.h", false))
config:Add(OptLibrary("curl", "curl/curl.h", false))
config:Finalize("config.lua")

settings = NewSettings()

-- Helper function for scripts
function Script(name)
    return "python3 " .. name
end

-- CHash function for generating hash files
function CHash(output, ...)
    local inputs = TableFlatten({...})
    output = Path(output)
    
    local cmd = Script("datasrc/cmd5.py")
    for index, inname in ipairs(inputs) do
        cmd = cmd .. " " .. Path(inname)
    end
    cmd = cmd .. " > " .. output
    
    AddJob(output, "cmd5 " .. output, cmd)
    for index, inname in ipairs(inputs) do
        AddDependency(output, inname)
    end
    AddDependency(output, "datasrc/cmd5.py")
    return output
end

-- Resource compilation for Windows
function ResCompile(scriptfile)
    -- Not used on Linux
    return nil
end

-- Content generation function
function ContentCompile(action, output)
    output = Path(output)
    AddJob(
        output,
        action .. " > " .. output,
        Script("datasrc/compile.py") .. " " .. action .. " > " .. output
    )
    AddDependency(output, Path("datasrc/content.py"))
    AddDependency(output, Path("datasrc/network.py"))
    AddDependency(output, Path("datasrc/compile.py"))
    AddDependency(output, Path("datasrc/datatypes.py"))
    return output
end

-- Git revision generation
function GitRevision(output)
    output = Path(output)
    AddJob(
        output,
        "git_revision > " .. output,
        Script("scripts/git_revision.py") .. " > " .. output
    )
    AddDependency(output, Path("scripts/git_revision.py"))
    return output
end

-- Generate protocol and content files
network_source = ContentCompile("network_source", "src/game/generated/protocol.cpp")
network_header = ContentCompile("network_header", "src/game/generated/protocol.h")
git_revision = GitRevision("src/game/generated/git_revision.cpp")

AddDependency(network_source, network_header)

-- Generate nethash
nethash = CHash(
    "src/game/generated/nethash.cpp",
    "src/engine/shared/protocol.h",
    "src/game/generated/protocol.h",
    "src/game/tuning.h",
    "src/game/gamecore.cpp"
)
AddDependency(nethash, network_header)

-- Custom intermediate output function
function Intermediate_Output(settings, input)
    return "objs/" .. string.sub(PathBase(input), string.len("src/") + 1) .. settings.config_ext
end

-- Apply compiler settings
config.compiler:Apply(settings)
settings.cc.Output = Intermediate_Output

-- Compiler flags (Linux GCC/Clang only)
settings.cc.flags:Add("-std=c++20", "-Wall", "-Wextra")
settings.cc.flags:Add("-Wno-unused-parameter", "-Wno-missing-field-initializers")
settings.cc.flags:Add("-fstack-protector-all")

-- Include directories
settings.cc.includes:Add("src")
settings.cc.includes:Add("src/engine/external/md5")
settings.cc.includes:Add("src/engine/external/json")

-- Force MySQL C++ Connector support (Linux only)
settings.cc.defines:Add("CONF_SQL")

-- Try pkg-config first (for newer versions)
if ExecuteSilent("pkg-config --exists mysqlcppconn8") == 0 then
    settings.cc.flags:Add("`pkg-config --cflags mysqlcppconn8`")
    settings.link.flags:Add("`pkg-config --libs mysqlcppconn8`")
    print("MySQL C++ Connector 8 configured via pkg-config")
elseif ExecuteSilent("pkg-config --exists mysqlcppconn") == 0 then
    settings.cc.flags:Add("`pkg-config --cflags mysqlcppconn`")
    settings.link.flags:Add("`pkg-config --libs mysqlcppconn`")
    print("MySQL C++ Connector configured via pkg-config")
else
    -- Add include paths for the old connector
    if ExecuteSilent("test -d /usr/include/cppconn") == 0 then
        settings.cc.includes:Add("/usr/include/cppconn")
    end
    -- Link the old connector library
    settings.link.libs:Add("mysqlcppconn")
    settings.link.libs:Add("mysqlclient")
end

-- Linux system libraries
settings.link.libs:Add("pthread", "rt")

-- Zlib and Curl
settings.link.libs:Add("z")
settings.link.libs:Add("curl")
settings.link.libs:Add("ssl", "crypto")

-- Compile MD5 library
md5_settings = settings:Copy()
md5 = Compile(md5_settings, Collect("src/engine/external/md5/*.c"))

-- Compile JSON parser
json_settings = settings:Copy()
json = Compile(json_settings, Collect("src/engine/external/json/*.c"))

-- Compile engine shared
engine_shared_settings = settings:Copy()
engine_shared = Compile(
    engine_shared_settings,
    Collect("src/base/*.cpp"),
    Collect("src/engine/shared/*.cpp")
)

-- Compile game shared
game_shared_settings = settings:Copy()
game_shared = Compile(
    game_shared_settings,
    Collect("src/game/*.cpp"),
    network_source,
    nethash,
    git_revision
)

-- Compile engine server
engine_server_settings = settings:Copy()
engine_server = Compile(
    engine_server_settings,
    Collect("src/engine/server/*.cpp")
)

-- Compile game server
game_server_settings = settings:Copy()
game_server = Compile(
    game_server_settings,
    CollectRecursive("src/game/server/*.cpp"),
    server_content_source
)

-- Link server executable
server_settings = settings:Copy()

-- Add link flags directly to ensure they're included
server_settings.link.flags:Add("-lz", "-lpthread", "-lrt", "-lcurl", "-lssl", "-lcrypto")
server_settings.link.flags:Add("-lmysqlcppconn", "-lmysqlclient")

server_exe = Link(
    server_settings,
    "teeworlds_srv",
    md5,
    json,
    engine_shared,
    game_shared,
    engine_server,
    game_server
)

-- Create pseudo targets
server_target = PseudoTarget("server", server_exe)
default_target = PseudoTarget("all", server_target)

DefaultTarget(default_target)
return default_target