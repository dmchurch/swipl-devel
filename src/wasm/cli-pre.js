Error.stackTraceLimit = 50;
var isInitialized = false;
var inStackTrace = false;
Module = {
    preInit: function() {
        ENV = process.env;
        var orig_exit = exit;
        exit = function(status) {return orig_exit(status, true);}; // avoid the "main thread called exit" message when multithreaded
        NODEFS.flagsForNodeMap[65536] = fs.constants.O_DIRECTORY;
        NODERAWFS.readdir = function() {return fs.readdirSync.apply(void 0, arguments)};
        FS.readdir = _wrapNodeError(NODERAWFS.readdir);
        SYSCALLS.getStreamFromFD = function(fd) {
            var stream = FS.getStream(fd);
            if (!stream) throw new FS.ErrnoError(8);
            if ((stream.flags & 65536) && !stream.node && stream.path) {
                const lookup = FS.lookupPath(stream.path);
                if (!lookup) throw new FS.ErrnoError(8);
                stream.node = Object.assign({}, lookup.node, {
                    path: stream.path,
                });
            }
            return stream;
        };
        FS.lookupNode = function(parent, name) {
            if (parent.path) {
                return FS.lookupPath(NODEJS_PATH.join(parent.path, name));
            }
            return VFS.lookupNode(parent, name);
        };
    },
    onRuntimeInitialized: function() {
        isInitialized = true;
    },
    extraStackTrace: function() {
        if (!isInitialized) return "";
        if (inStackTrace) return "(recursive call)\n";
        inStackTrace = true;
        try {
            var bt = "";
            var bts = cwrap("PL_backtrace_string", "string", [ "number", "number" ]);
            if (!bt) try {
                bt = bts(20, 1 /*PL_BT_SAFE*/);
                if (bt) return "Prolog backtrace (safe):\n";
            } catch (e) {
                return "Could not get Prolog backtrace: " + (e.message || e.toString());
            }
            return "No Prolog backtrace available\n";
        } finally {
            inStackTrace = false;
        }
    },
};
