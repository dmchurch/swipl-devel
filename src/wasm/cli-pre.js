Error.stackTraceLimit = 50;
var isInitialized = false;
var inStackTrace = false;
Object.assign(Module, {
    preInit: function() {
        ENV = process.env;
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
});
