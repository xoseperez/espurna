import capitalize from "./capitalize";

const flatten = (obj, mask, prefix, suffix, flat) => {
    flat = typeof flat === "undefined" ? {} : flat;

    obj = obj || {};
    prefix = prefix || "";
    suffix = suffix || "";
    mask = mask || {};

    Object.keys(obj).forEach((key) => {
        if (!("_" + key in mask)) {
            let Key = prefix !== "" ? capitalize(key) : key;
            if ("_path" in mask) {
                Key = mask._path;
            }


            let v = obj[key];

            if (typeof mask[key] === "object" && typeof v === "undefined" && key in obj) {
                v = {...mask[key]};
                Object.keys(v).forEach((k) => {
                    if (k.charAt(0) === "_") {
                        delete v[k];
                    } else {
                        v[k] = null;
                    }
                });
            }

            if (typeof v === "object" && v !== null) {
                flatten(v, mask[key], Array.isArray(mask[key]) || Array.isArray(mask) ? prefix : (prefix + Key),
                    Array.isArray(mask) ? suffix + key : suffix,
                    flat);
            } else {
                flat[prefix + Key + suffix] = v;
            }
        }
    });
    return flat;
};

export default flatten;
