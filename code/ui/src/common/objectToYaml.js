const yamlEscape = (value) => {
    if (typeof value === "string" && value.match(/^(y|n|yes|no|true|false|on|off)$/i)) {
        return "\"" + value + "\"";
    }
    return value.toString();
};

/**
 * Ultra simplistic objectToYaml converter
 *
 * @var obj Object to convert
 * @var lvl Integer Optional level to start at
 *
 * @return string
 */
const objectToYaml = (obj, lvl) => {
    lvl = lvl || 0;

    let str = "";
    let first = true;

    const isArray = Array.isArray(obj);

    for (let key in obj) {
        if (obj.hasOwnProperty(key)) {
            let value = isArray ? key : obj[key];
            if (typeof value === "object") {
                str += objectToYaml(value, lvl + 1);
            } else {
                str += " ".repeat(lvl * 2);
                if (isArray || (first && lvl)) {
                    str += "- ";
                }
                str += key + ": " + yamlEscape(value) + "\n";
            }
        }
    }
    return str;
};

export default objectToYaml;
