%% if the loglevel is never set it defaults to debug
function setLogLevel(level)
    arguments
        level {mustBeMember(level, ["debug", "info", "warn", "error"])}
    end
    level_id = 0;
    if level == "debug"
        level_id = 0;
    elseif level == "info"
            level_id = 1;
    elseif level == "warn"
            level_id = 2;
    elseif level == "error"
            level_id = 3;
    end
    MexGateway("none.setLogLevel", int32(level_id));
end