import Vue from "vue";

function isObject(item) {
    return (item && typeof item === "object" && !Array.isArray(item));
}
const prepareData = (target, source) => {
    Object.keys(source).forEach((k) => {
        let val = source[k];

        if (isObject(val)) {
            if (!target[k]) {
                Vue.set(target, k, {});
            }

            if (val._schema && Array.isArray(val.list)) {
                let objs = [];

                val.list.forEach((v) => {
                    if (Array.isArray(v)) {
                        let i = 0;
                        let obj = {};
                        v.forEach((prop) => {
                            obj[val._schema[i++]] = prop;
                        });
                        objs.push(obj);
                    }
                });

                if (val.start) {
                    val.list = [...target.list];
                    val.list.splice(val.start, objs.length, ...objs);
                } else {
                    val.list = objs;
                }
            }

            prepareData(target[k], val);
        } else {
            Vue.set(target, k, val);
        }
    });
};

export default prepareData;
