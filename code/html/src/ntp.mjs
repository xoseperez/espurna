import { addEnumerables } from './settings/enumerable.mjs';

export function init() {
    /** @import { EnumerableNames } from './settings/enumerable.mjs' */

    /** @type {EnumerableNames} */
    const timezones = {"UTC0": "No offset"};

    for (let offset = -14; offset < 13; ++offset) {
        timezones[`UTC${offset}`] = "";
    }

    addEnumerables("timezones", timezones);
}
