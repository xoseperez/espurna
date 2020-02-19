// #!if PWA
import Vue from "vue";
// #!else
const _text = (msg) => {
    let str = msg;
    if (typeof msg === "object") {
        str = "";
        switch (msg.type) {
            case "warning":
                str = "Warning: ";
                break;
            case "error":
                str = "Error: ";
                break;
            case "success":
                str = "Success: ";
                break;
        }
        str += msg.title + " \n" + msg.message;
    }
    return str;
};
// #!endif

const _alert = (msg) => {
    // #!if PWA
    Vue.$notify(msg);
    // #!else
    alert(_text(msg));
    // #!endif
};

const alertError = (message) => {
    _alert(typeof message === "object" ? {type: "error", ...message} : {type: "error", message});
};
const alertSuccess = (message) => {
    _alert(typeof message === "object" ? {type: "success", ...message} : {type: "success", message});
};

const alertInfo = (message) => {
    _alert(typeof message === "object" ? {type: "info", ...message} : {type: "info", message});
};

const alertWarning = (message) => {
    _alert(typeof message === "object" ? {type: "warning", ...message} : {type: "warning", message});
};

const confirm = (msg) => {
    // #!if PWA
    return typeof msg === "string" ? Vue.$confirm(msg) : Vue.$confirm(msg.message, msg.title, msg);
    // #!else
    return new Promise((resolve, fail) => {
        confirm(_text(msg)) ? resolve() : fail();
    });
    // #!endif
};


export {
    alertError,
    alertSuccess,
    alertInfo,
    alertWarning,
    confirm
};
