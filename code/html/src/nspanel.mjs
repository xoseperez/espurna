import { listenAppConnected } from './connection.mjs';
import { styleInject } from './core.mjs';

// <link> stylesheets are not stripped per-module by the build, keep the rules
// here so that only the variant(s) built with MODULE_NSPANEL carry them
const STYLE = [
    `.nspanel-editor { font-family: 'Courier New', monospace; font-size: 85%; line-height: 1.3;
        height: 480px; resize: vertical; tab-size: 2; white-space: pre; }`,
    ".nspanel-buttons .pure-button { margin-right: 6px; }",
    ".nspanel-result { margin: 0; white-space: pre-wrap; word-break: break-word; }",
    ".nspanel-result.nspanel-ok { color: #1b7f1b; }",
    ".nspanel-result.nspanel-error { color: #c0392b; }",
    ".nspanel-card { border: 1px solid #ddd; border-radius: 6px; padding: 8px; margin: 0 0 12px; max-width: 520px; }",
    ".nspanel-card-title { font-weight: bold; margin: 0 0 6px; }",
    ".nspanel-card.nspanel-nested .nspanel-card-title { font-weight: normal; font-style: italic; }",
    ".nspanel-grid { display: grid; gap: 6px; grid-template-columns: repeat(4, 1fr); }",
    ".nspanel-grid.nspanel-shortcuts { grid-template-columns: repeat(7, 1fr); }",
    `.nspanel-tile { background: #f2f2f2; border-radius: 6px; padding: 6px 4px; text-align: center;
        font-size: 12px; min-height: 48px; overflow: hidden; word-break: break-word; }`,
    ".nspanel-tile.nspanel-bad { outline: 1px solid #c0392b; }",
    ".nspanel-tile-icon { color: #777; font-size: 10px; }",
    ".nspanel-tile-name { margin-top: 3px; }",
    ".nspanel-tile-type { color: #1f8dd6; font-size: 10px; }",
    ".nspanel-tile.nspanel-empty { background: transparent; border: 1px dashed #e4e4e4; }",
    ".nspanel-rows { display: flex; flex-direction: column; gap: 4px; }",
    `.nspanel-row { display: flex; gap: 8px; align-items: baseline; background: #f2f2f2; border-radius: 6px;
        padding: 6px 8px; font-size: 12px; }`,
    ".nspanel-row.nspanel-bad { outline: 1px solid #c0392b; }",
    ".nspanel-row .nspanel-tile-icon { flex: 0 0 25%; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }",
    ".nspanel-row .nspanel-tile-name { flex: 1 1 auto; margin: 0; }",
    `.nspanel-palette { position: sticky; top: 0; z-index: 1; display: flex; flex-wrap: wrap; gap: 6px; align-items: center;
        background: #fff; padding: 6px 0; margin: 0 0 8px; border-bottom: 1px solid #eee; }`,
    `.nspanel-chip { border: 1px solid #ccc; border-radius: 12px; padding: 3px 10px; font-size: 12px; background: #fafafa;
        cursor: grab; user-select: none; }`,
    ".nspanel-chip.nspanel-trash { border-style: dashed; border-color: #c0392b; color: #c0392b; cursor: default; }",
    ".nspanel-palette .pure-button { font-size: 12px; padding: 3px 10px; }",
    ".nspanel-card-head { display: flex; gap: 8px; align-items: center; justify-content: space-between; margin: 0 0 6px; }",
    ".nspanel-card-head .nspanel-card-title { margin: 0; flex: 1 1 auto; min-width: 0; }",
    ".nspanel-card-head select { flex: 0 0 auto; width: auto; font-size: 12px; padding: 1px 4px; height: auto; }",
    ".nspanel-card-title[draggable='true'], .nspanel-tile[draggable='true'], .nspanel-row[draggable='true'] { cursor: grab; }",
    ".nspanel-dragging { opacity: 0.4; }",
    ".nspanel-drop { outline: 2px dashed #1f8dd6 !important; outline-offset: 1px; }",
    ".nspanel-rename { width: 100%; box-sizing: border-box; font-size: 12px; padding: 1px 2px; }",
    ".nspanel-workspace { display: flex; flex-wrap: wrap; gap: 16px; align-items: flex-start; }",
    ".nspanel-workspace .nspanel-preview-area { flex: 1 1 360px; min-width: 0; max-width: 540px; }",
    `.nspanel-form { flex: 0 0 280px; position: sticky; top: 48px; border: 1px solid #1f8dd6; border-radius: 6px;
        padding: 8px 10px; font-size: 12px; background: #fff; max-height: calc(100vh - 64px); overflow-y: auto; }`,
    ".nspanel-form:empty { display: none; }",
    ".nspanel-form-head { display: flex; justify-content: space-between; align-items: center; gap: 8px; margin: 0 0 6px; }",
    ".nspanel-form-head .pure-button, .nspanel-form .nspanel-remove { font-size: 12px; padding: 2px 8px; }",
    ".nspanel-form .nspanel-remove { margin-top: 8px; background: #c0392b; color: #fff; }",
    ".nspanel-field { display: block; margin: 0 0 6px; }",
    ".nspanel-field span { display: block; color: #777; margin: 0 0 1px; }",
    `.nspanel-field input, .nspanel-field select, .nspanel-field textarea { width: 100%; box-sizing: border-box; font-size: 12px;
        padding: 2px 4px; height: auto; }`,
    ".nspanel-field textarea { white-space: pre; overflow-x: auto; resize: vertical; font-family: 'Courier New', monospace; }",
    ".nspanel-selected { outline: 2px solid #1f8dd6; outline-offset: 1px; }",
];

// tiles per screen, OHpanel and Lovelace TFT >= 51 (a 4x2 grid of slots)
const PREVIEW_CAPACITY = 8;

// OHpanel custom TFT design with a page per grid layout (buttons33 / buttons32 / buttons22 / buttons12)
export const DESIGN_GRIDS = "d3731fbb";

// OHpanel designs that show every layout in full: the custom one on its pages, the stock ones
// (v0.9.2, v0.10.0-rc1) drawn at runtime on `buttons`
export const DESIGNS_FULL_GRIDS = [DESIGN_GRIDS, "bc629e1e", "4508cd3f"];

/**
 * "layout" of a grid page: [columns, rows]
 * @type {{[k: string]: [number, number]}}
 */
export const GRIDS = {
    "4x2": [4, 2],
    "3x3": [3, 3],
    "3x2": [3, 2],
    "2x2": [2, 2],
    "1x2": [1, 2],
};

/** preview full grids (OHpanel custom pages or drawn tiles), until the status names an unknown design */
let __grids = true;

/** icon names of the panel's built-in table, from its status */
let __icons = /** @type {string[]} */ ([]);

// rows per sheet of a "type": "list" page on the OHpanel entities_rows page
export const LIST_ROWS = 7;

/**
 * device endpoints, relative to the page unless the connection says otherwise (?host=...)
 * @typedef {{config: URL, status: URL}} NspanelUrls
 */

/** @type {NspanelUrls | null} */
let __urls = null;

let __loaded = false;

/** @type {{[k in keyof NspanelUrls]: string}} */
const RELATIVE = {
    config: "nspanel.json",
    status: "nspanel-status",
};

/**
 * @param {keyof NspanelUrls} name
 * @returns {string}
 */
function endpoint(name) {
    return __urls ? __urls[name].href : RELATIVE[name];
}

/** @type {RequestInit} */
const FETCH_OPTIONS = {
    credentials: "same-origin",
    cache: "no-store",
};

/**
 * 1-based line and column of the offset in text
 * @param {string} text
 * @param {number} offset
 * @returns {{line: number, column: number}}
 */
export function offsetToLineColumn(text, offset) {
    const before = text.slice(0, Math.max(0, offset));
    const lines = before.split("\n");

    return {
        line: lines.length,
        column: (lines[lines.length - 1] ?? "").length + 1,
    };
}

/**
 * @param {string} text
 * @param {number} line
 * @param {number} column
 * @returns {number}
 */
function lineColumnToOffset(text, line, column) {
    let offset = 0;
    for (let current = 1; current < line; ++current) {
        const next = text.indexOf("\n", offset);
        if (next < 0) {
            return text.length;
        }

        offset = next + 1;
    }

    return Math.min(text.length, offset + Math.max(0, column - 1));
}

/**
 * offset of the first syntax error, for messages w/o position
 * (V8 'Unexpected token' only quotes the surrounding text)
 * @param {string} text
 * @returns {number}
 */
export function jsonErrorOffset(text) {
    let pos = 0;

    const fail = () => {
        throw pos;
    };

    const ws = () => {
        while ((pos < text.length) && " \t\n\r".includes(text[pos])) {
            ++pos;
        }
    };

    /** @param {string} expected */
    const take = (expected) => {
        if (text[pos] !== expected) {
            fail();
        }
        ++pos;
    };

    const string = () => {
        take('"');
        while (pos < text.length) {
            const c = text.charCodeAt(pos);
            if (c === 0x22) {
                ++pos;
                return;
            }
            if (c < 0x20) {
                fail();
            }
            if (c === 0x5c) {
                ++pos;
                if (text[pos] === "u") {
                    if (!/^[0-9a-fA-F]{4}$/.test(text.slice(pos + 1, pos + 5))) {
                        fail();
                    }
                    pos += 4;
                } else if (!"\"\\/bfnrt".includes(text[pos] ?? "?")) {
                    fail();
                }
            }
            ++pos;
        }
        fail();
    };

    /** @returns {void} */
    const value = () => {
        ws();
        const c = text[pos];
        if (c === "{" || c === "[") {
            const close = (c === "{") ? "}" : "]";
            ++pos;
            ws();
            if (text[pos] === close) {
                ++pos;
                return;
            }
            for (;;) {
                if (c === "{") {
                    ws();
                    string();
                    ws();
                    take(":");
                }
                value();
                ws();
                if (text[pos] === ",") {
                    ++pos;
                    continue;
                }
                take(close);
                return;
            }
        }
        if (c === '"') {
            string();
            return;
        }
        const literal = text.slice(pos).match(/^(?:true|false|null|-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)/);
        if (!literal) {
            fail();
        } else {
            pos += literal[0].length;
        }
    };

    try {
        value();
        ws();
        if (pos < text.length) {
            fail();
        }
    } catch (e) {
        if (typeof e === "number") {
            return e;
        }
        throw e;
    }

    return -1;
}

/**
 * @typedef {{ok: true} | {ok: false, message: string, line?: number, column?: number, offset?: number}} ValidateResult
 */

/**
 * JSON syntax check, error position from either browser message format
 * - Chrome / Node: '... in JSON at position 42 (line 3 column 5)' or only '... at position 42'
 * - Firefox: '... at line 3 column 5 of the JSON data'
 * @param {string} text
 * @returns {ValidateResult}
 */
export function validateJson(text) {
    if (!text.trim().length) {
        return {ok: false, message: "empty configuration"};
    }

    try {
        const parsed = JSON.parse(text);
        if ((parsed === null) || (typeof parsed !== "object") || Array.isArray(parsed)) {
            return {ok: false, message: "top level value must be a JSON object"};
        }
    } catch (e) {
        const message = (e instanceof Error) ? e.message : String(e);

        const lineColumn = message.match(/line (\d+) column (\d+)/);
        if (lineColumn) {
            const line = parseInt(lineColumn[1], 10);
            const column = parseInt(lineColumn[2], 10);
            return {
                ok: false, message, line, column,
                offset: lineColumnToOffset(text, line, column),
            };
        }

        const position = message.match(/position (\d+)/);
        if (position) {
            const offset = parseInt(position[1], 10);
            return {ok: false, message, offset, ...offsetToLineColumn(text, offset)};
        }

        let offset = jsonErrorOffset(text);
        if (offset < 0) {
            offset = text.length;
        }

        return {ok: false, message, offset, ...offsetToLineColumn(text, offset)};
    }

    return {ok: true};
}

/**
 * @param {ValidateResult} result
 * @returns {string}
 */
export function formatValidateResult(result) {
    if (result.ok) {
        return "JSON is valid";
    }

    if ((result.line !== undefined) && (result.column !== undefined)) {
        return `Line ${result.line}, column ${result.column}: ${result.message}`;
    }

    return result.message;
}

/**
 * list: rows of a "type": "list" page; grid: the page's "layout" ("4x2", "3x3", "3x2", "2x2", "1x2");
 * depth: 0 for the swipe row (top-level "pages"), 1+ for a nested page (an item's "page", opened by its tile);
 * page: the page object the screen shows, offset: index of its first item in the page's "items"
 * @typedef {{title: string, items: any[], list: boolean, grid: string, depth: number, page: any, offset: number}} PreviewCard
 */

/**
 * an item's nested page ("page": {...}), null when it has none
 * @param {any} item
 * @returns {any}
 */
export function nestedPage(item) {
    const page = item?.page;
    return ((typeof page === "object") && (page !== null) && !Array.isArray(page)) ? page : null;
}

/**
 * the name a tile shows: its "name", else the title of the page it opens
 * @param {any} item
 * @returns {string}
 */
export function itemName(item) {
    if (typeof item === "string") {
        return item;
    }
    const name = String(item?.name ?? "");
    return name.length ? name : String(nestedPage(item)?.title ?? "");
}

/**
 * "layout" as the firmware takes it, else "columns": 3 = "3x2", else "4x2"
 * @param {any} page
 * @returns {string}
 */
export function pageGrid(page) {
    const layout = page?.layout;
    if ((typeof layout === "string") && Object.prototype.hasOwnProperty.call(GRIDS, layout)) {
        return layout;
    }

    return (page?.columns === 3) ? "3x2" : "4x2";
}

/**
 * "per_screen" as the firmware takes it: 1..9, anything else is 0 (as many as fit)
 * @param {any} page
 * @returns {number}
 */
function pagePerScreen(page) {
    const value = page?.per_screen;
    return (Number.isInteger(value) && (value >= 1) && (value <= 9)) ? value : 0;
}

/**
 * tiles or rows of a page on one screen, as nspanel.cpp gridCapacity() / buildCards()
 * - grids (OHpanel, known design): columns x rows tiles (own page or drawn)
 * - else the layout's columns of the 4x2 slots, at most the driver's capacity
 * @param {any} page
 * @param {number} capacity
 * @param {boolean} list
 * @param {boolean} grids
 * @returns {number}
 */
export function perScreen(page, capacity = PREVIEW_CAPACITY, list = false, grids = false) {
    const per = pagePerScreen(page);
    if (list) {
        return per ? Math.min(per, LIST_ROWS) : LIST_ROWS;
    }

    const [columns, rows] = GRIDS[pageGrid(page)];
    let out = grids ? (columns * rows) : Math.min(capacity, 2 * columns);
    if (per) {
        out = Math.min(out, per);
    }
    return Math.max(out, 1);
}

/**
 * screens as the panel shows them, long pages split into 'Title 1/2', 'Title 2/2'; the nested pages
 * of a page's tiles follow its sheets (depth + 1), they are not in the swipe row
 * @param {any} config
 * @param {number} capacity tiles of a full 4x2 grid screen
 * @param {boolean} lists "type": "list" pages as rows (OHpanel); false: as grids (Lovelace)
 * @param {boolean} grids the TFT has a page per layout (OHpanel custom design)
 * @returns {PreviewCard[]}
 */
export function previewCards(config, capacity = PREVIEW_CAPACITY, lists = true, grids = false) {
    /** @type {PreviewCard[]} */
    const out = [];

    /**
     * @param {any} page
     * @param {number} depth
     * @param {string} fallback title when the page has none (the name of its tile)
     */
    const add = (page, depth, fallback) => {
        const title = String(page?.title ?? "") || fallback;
        if (page?.type === "settings") {
            out.push({title: title || "Settings", items: [], list: false, grid: "4x2", depth, page, offset: 0});
            return;
        }

        const list = lists && (page?.type === "list");
        const per = perScreen(page, capacity, list, grids);
        const grid = list ? "4x2" : pageGrid(page);
        const items = Array.isArray(page?.items) ? page.items : [];
        const count = Math.max(1, Math.ceil(items.length / per));
        for (let index = 0; index < count; ++index) {
            out.push({
                title: (count > 1) ? `${title} ${index + 1}/${count}` : title,
                items: items.slice(index * per, (index + 1) * per),
                list,
                grid,
                depth,
                page,
                offset: index * per,
            });
        }

        if (depth < 8) {
            for (const item of items) {
                const nested = nestedPage(item);
                if (nested) {
                    add(nested, depth + 1, String(item?.name ?? ""));
                }
            }
        }
    };

    const pages = Array.isArray(config?.pages) ? config.pages : [];
    for (const page of pages) {
        add(page, 0, "");
    }

    return out;
}

/**
 * titles of the swipe row after home, as the panel swipes them (nested pages are not in it)
 * @param {any} config
 * @param {number} capacity
 * @param {boolean} lists
 * @param {boolean} grids
 * @returns {string[]}
 */
export function previewRow(config, capacity = PREVIEW_CAPACITY, lists = true, grids = false) {
    return previewCards(config, capacity, lists, grids)
        .filter((card) => !card.depth)
        .map((card) => card.title);
}

/**
 * the tile slots of a grid card, null where no tile is shown, and the columns they are drawn in
 * - grids (OHpanel, known design): columns x rows slots (own page or drawn)
 * - else the 4x2 slots of the `buttons` page (tile1..tile8), the layout's columns of each row:
 *   3x3 / 3x2 fill slots 1-3 and 5-7, 2x2 slots 1, 2, 5, 6, 1x2 slots 1 and 5
 * indexes: per slot the index of its item in card.items, -1 for an empty slot
 * @param {PreviewCard} card
 * @param {boolean} grids
 * @returns {{columns: number, slots: any[], indexes: number[]}}
 */
export function previewGrid(card, grids = false) {
    const [columns, rows] = GRIDS[card.grid] ?? GRIDS["4x2"];
    if (grids) {
        /** @type {any[]} */
        const slots = Array(columns * rows).fill(null);
        const indexes = Array(slots.length).fill(-1);
        card.items.slice(0, slots.length)
            .forEach((item, index) => {
                slots[index] = item;
                indexes[index] = index;
            });
        return {columns, slots, indexes};
    }

    /** @type {any[]} */
    const slots = Array(PREVIEW_CAPACITY).fill(null);
    const indexes = Array(PREVIEW_CAPACITY).fill(-1);
    card.items.slice(0, 2 * columns)
        .forEach((item, index) => {
            const slot = (Math.floor(index / columns) * 4) + (index % columns);
            slots[slot] = item;
            indexes[slot] = index;
        });

    return {columns: 4, slots, indexes};
}

/**
 * true when list is the "items" of the item's nested page, or of a page nested deeper in it
 * (moving the item there would put it inside itself)
 * @param {any} item
 * @param {any[]} list
 * @returns {boolean}
 */
export function itemContains(item, list) {
    const items = nestedPage(item)?.items;
    if (!Array.isArray(items)) {
        return false;
    }

    return (items === list) || items.some((child) => itemContains(child, list));
}

/**
 * moves an item (or a page) to another place: it takes the place of the one at `to` (that one and the
 * following move on), an index past the end appends; false when nothing moved
 * @param {any[]} from
 * @param {number} fromIndex
 * @param {any[]} to
 * @param {number} toIndex
 * @returns {boolean}
 */
export function moveItem(from, fromIndex, to, toIndex) {
    if ((fromIndex < 0) || (fromIndex >= from.length)) {
        return false;
    }

    const item = from[fromIndex];
    if ((from !== to) && itemContains(item, to)) {
        return false;
    }

    from.splice(fromIndex, 1);
    const index = Math.max(0, Math.min(toIndex, to.length));
    to.splice(index, 0, item);

    return (from !== to) || (index !== fromIndex);
}

/**
 * the name a tile shows, as typed in the editor: an empty name falls back to the nested page title;
 * a string item (a reference by name) is replaced, unless the name is empty
 * @param {any[]} list
 * @param {number} index
 * @param {string} name
 */
export function renameItem(list, index, name) {
    const item = list[index];
    if (typeof item === "string") {
        if (name.length) {
            list[index] = name;
        }
        return;
    }

    if ((typeof item !== "object") || (item === null)) {
        return;
    }

    if (name.length) {
        item.name = name;
    } else {
        delete item.name;
    }
}

/**
 * what the editor palette adds; empty targets are outlined until they are filled in the JSON
 * @type {{[k: string]: {label: string, make: () => any}}}
 */
export const TEMPLATES = {
    switch: {label: "Switch", make: () => ({name: "Switch", icon: "lightbulb", topic: ""})},
    button: {label: "Button", make: () => ({name: "Button", icon: "gesture-tap-button", type: "button", topic: ""})},
    scene: {label: "Scene", make: () => ({name: "Scene", icon: "lightbulb-group-off", type: "scene", group: "", action: "toggle"})},
    thermostat: {label: "Thermostat", make: () => ({name: "Thermostat", icon: "radiator", type: "thermostat", topic: "", room: ""})},
    text: {label: "Text", make: () => ({name: "Text", icon: "thermometer", type: "text", topic: ""})},
    url: {label: "URL", make: () => ({name: "URL", icon: "power", url: ""})},
    subpage: {label: "Sub-page", make: () => ({name: "Page", icon: "view-grid", page: {layout: "2x2", items: []}})},
    settings: {label: "Settings", make: () => ({name: "Settings", icon: "cog", open: "settings"})},
};

/** a new page of the swipe row */
export function newPage() {
    return {title: "New page", items: []};
}

/**
 * what a tile does, as the settings form offers it
 * @type {{[k: string]: string}}
 */
export const KINDS = {
    switch: "Switch (MQTT topic)",
    button: "Button (sends on once)",
    relay: "Relay of the panel",
    url: "Calls a URL (HTTP GET)",
    scene: "Scene (a group)",
    text: "Text (shows a value)",
    thermostat: "Thermostat",
    page: "Opens its nested page",
    open: "Opens another page",
};

/**
 * a field of the settings form
 * - type: text | number | path (comma separated keys, digits index an array) | numpath (a number or a path)
 *   | numtext (a number or a text) | bool | members (a text box, one state topic or relay number a line)
 * - redraw: the form is drawn again after a change (other fields depend on it)
 * - options: a select of these values (plus "" = not set); choices: the same for a list the panel reports
 *   ("icons"), the current value stays selectable; suggest: free text with these proposals
 * - nested: the field belongs to the item's nested page
 * @typedef {{key: string, label: string, type?: string, options?: string[], choices?: string, suggest?: string, nested?: boolean, redraw?: boolean}} Field
 */

/** @type {Field[]} */
export const COMMON_FIELDS = [
    {key: "name", label: "Name"},
    {key: "icon", label: "Icon", choices: "icons"},
    {key: "color", label: "Icon colour when on (#rrggbb)"},
];

/** @type {{[k: string]: Field[]}} */
export const KIND_FIELDS = {
    switch: [
        {key: "topic", label: "State topic"},
        {key: "path", label: "Path in a JSON state", type: "path"},
        {key: "on", label: "On value"},
        {key: "off", label: "Off value"},
        {key: "command", label: "Command topic (default: topic + /set)"},
        {key: "payload", label: "Command payload, %s = value"},
        {key: "history", label: "History topic (a tap opens the chart instead of switching)"},
    ],
    button: [
        {key: "topic", label: "State topic"},
        {key: "path", label: "Path in a JSON state", type: "path"},
        {key: "on", label: "Value sent"},
        {key: "command", label: "Command topic (default: topic + /set)"},
        {key: "payload", label: "Command payload, %s = value"},
    ],
    relay: [
        {key: "relay", label: "Relay number (0 = first)", type: "number"},
        {key: "history", label: "History topic (a tap opens the chart instead of switching)"},
    ],
    url: [
        {key: "url", label: "URL called on every tap (http:// or https://)"},
        {key: "on_url", label: "URL to switch on (instead)"},
        {key: "off_url", label: "URL to switch off (instead)"},
        {key: "topic", label: "State topic (optional, else the panel keeps the state)"},
        {key: "path", label: "Path in a JSON state", type: "path"},
        {key: "on", label: "On value"},
        {key: "off", label: "Off value"},
    ],
    scene: [
        {key: "group", label: "Group (its members are below)", suggest: "groups", redraw: true},
        {key: "action", label: "Action", options: ["toggle", "on", "off"]},
    ],
    text: [
        {key: "topic", label: "State topic"},
        {key: "path", label: "Path in a JSON state", type: "path"},
    ],
    thermostat: [
        {key: "topic", label: "State topic (JSON)"},
        {key: "room", label: "Room (Istabai key)"},
        {key: "command", label: "Command topic"},
        {key: "payload", label: "Command payload, %s = setpoint"},
        {key: "step", label: "Step", type: "number"},
        {key: "min", label: "Minimum (number or path)", type: "numpath"},
        {key: "max", label: "Maximum (number or path)", type: "numpath"},
        {key: "current", label: "Path: current temperature", type: "path"},
        {key: "setpoint", label: "Path: setpoint", type: "path"},
        {key: "active", label: "Path: heating", type: "path"},
        {key: "humidity", label: "Path: humidity", type: "path"},
        {key: "history", label: "History topic"},
    ],
    page: [
        {key: "title", label: "Page title (default: the tile name)", nested: true},
        {key: "layout", label: "Page layout", options: Object.keys(GRIDS), nested: true},
    ],
    open: [
        {key: "open", label: "Page: title, id, number, weather, home, settings", type: "numtext", suggest: "pages"},
    ],
};

/** @type {Field[]} */
export const PAGE_FIELDS = [
    {key: "title", label: "Title"},
    {key: "id", label: "Id (for \"open\")"},
    {key: "type", label: "Type", options: ["grid", "list", "settings"]},
    {key: "layout", label: "Layout", options: Object.keys(GRIDS)},
    {key: "per_screen", label: "Tiles per screen (1..9)", type: "number"},
    {key: "draw", label: "Draw the tiles (OHpanel)", type: "bool"},
];

/**
 * what a tile does: one of KINDS, "ref" for a reference by name (a string)
 * @param {any} item
 * @returns {string}
 */
export function itemKind(item) {
    if (typeof item === "string") {
        return "ref";
    }
    if (item?.page !== undefined) {
        return "page";
    }
    if (item?.open !== undefined) {
        return "open";
    }
    if (item?.relay !== undefined) {
        return "relay";
    }
    if ((item?.url !== undefined) || (item?.on_url !== undefined) || (item?.off_url !== undefined)) {
        return "url";
    }

    const type = String(item?.type ?? "switch");
    return Object.prototype.hasOwnProperty.call(KINDS, type) ? type : "switch";
}

/**
 * makes the item do something else: keeps the name, the icon, the colour and the fields the new kind
 * has too (a topic stays a topic), drops the other targets, adds the required ones
 * @param {any} item
 * @param {string} kind
 * @returns {any}
 */
export function setItemKind(item, kind) {
    const keep = new Set(["type", ...COMMON_FIELDS.map((field) => field.key)]);
    for (const field of (KIND_FIELDS[kind] ?? [])) {
        if (!field.nested) {
            keep.add(field.key);
        }
    }

    const targets = new Set(["page", "font"]);
    for (const fields of Object.values(KIND_FIELDS)) {
        for (const field of fields) {
            if (!field.nested) {
                targets.add(field.key);
            }
        }
    }

    for (const key of Object.keys(item)) {
        if (targets.has(key) && !keep.has(key)) {
            delete item[key];
        }
    }

    delete item.type;
    switch (kind) {
    case "button":
    case "text":
    case "thermostat":
        item.type = kind;
        item.topic = item.topic ?? "";
        break;
    case "scene":
        item.type = kind;
        item.group = item.group ?? "";
        item.action = item.action ?? "toggle";
        break;
    case "relay":
        item.relay = item.relay ?? 0;
        break;
    case "page":
        item.page = {items: []};
        break;
    case "open":
        item.open = item.open ?? "settings";
        break;
    case "url":
        if ((item.on_url === undefined) && (item.off_url === undefined)) {
            item.url = item.url ?? "";
        }
        break;
    default:
        item.topic = item.topic ?? "";
        break;
    }

    return item;
}

/**
 * "a, b, 0" -> ["a", "b", 0]: keys of a JSON path, a number indexes an array
 * @param {string} text
 * @returns {(string | number)[]}
 */
export function parsePath(text) {
    return text.split(",")
        .map((part) => part.trim())
        .filter((part) => part.length)
        .map((part) => /^\d+$/.test(part) ? parseInt(part, 10) : part);
}

/**
 * a field value as the form shows it
 * @param {any} value
 * @returns {string}
 */
export function formatValue(value) {
    if (value === undefined || value === null) {
        return "";
    }
    if (Array.isArray(value)) {
        return value.join(", ");
    }
    return String(value);
}

/**
 * sets target[key] from the text of a form field; an empty text removes the key
 * @param {any} target
 * @param {string} key
 * @param {string} type
 * @param {string} raw
 */
export function applyField(target, key, type, raw) {
    const text = raw.trim();
    if (!text.length) {
        delete target[key];
        return;
    }

    const number = Number(text);
    switch (type) {
    case "number":
        if (Number.isFinite(number)) {
            target[key] = number;
        }
        break;
    case "path":
        target[key] = parsePath(text);
        break;
    case "numpath":
        target[key] = Number.isFinite(number) ? number : parsePath(text);
        break;
    case "numtext":
        target[key] = (Number.isInteger(number) && /^\d+$/.test(text)) ? number : text;
        break;
    case "bool":
        target[key] = (text === "true");
        break;
    case "members":
        target[key] = text.split("\n")
            .map((line) => line.trim())
            .filter((line) => line.length)
            .map((line) => /^\d+$/.test(line) ? parseInt(line, 10) : line);
        break;
    default:
        target[key] = text;
        break;
    }
}

/**
 * the 4x2 tile slots of a grid card on the stock OHpanel design
 * @param {PreviewCard} card
 * @returns {any[]}
 */
export function previewSlots(card) {
    return previewGrid(card, false).slots;
}

/**
 * what a tile does, for the tooltip and the error outline
 * @param {any} item
 * @param {any} groups
 * @returns {{type: string, target: string, bad: boolean}}
 */
export function previewTarget(item, groups) {
    if (typeof item === "string") {
        return {type: "ref", target: item, bad: false};
    }

    const type = String(item?.type ?? "switch");
    if (type === "scene") {
        const group = String(item?.group ?? "");
        return {
            type,
            target: `group ${group} ${item?.action ?? "toggle"}`,
            bad: !groups || !(group in groups),
        };
    }

    if (type === "thermostat") {
        return {type, target: String(item?.room ?? item?.topic ?? ""), bad: !item?.topic};
    }

    const nested = nestedPage(item);
    if (nested) {
        return {type: "page", target: `page ${itemName(item)}`, bad: !Array.isArray(nested.items)};
    }

    if (item?.page !== undefined) {
        return {type: "page", target: "\"page\" must be an object", bad: true};
    }

    if (item?.open !== undefined) {
        return {type: "open", target: `open ${item.open}`, bad: false};
    }

    if (item?.relay !== undefined) {
        return {type, target: `relay ${item.relay}`, bad: false};
    }

    const url = String(item?.url || item?.on_url || item?.off_url || "");
    if (url.length) {
        return {type: "url", target: `GET ${url}`, bad: !/^https?:\/\//.test(url)};
    }

    const topic = String(item?.topic ?? "");
    return {type, target: topic, bad: !topic.length};
}

/**
 * @param {any} item
 * @param {any} groups
 * @param {boolean} row
 * @returns {HTMLElement}
 */
function previewTile(item, groups, row = false) {
    const tile = document.createElement("div");
    if (item === null) {
        tile.classList.add("nspanel-tile", "nspanel-empty");
        return tile;
    }

    const what = previewTarget(item, groups);
    tile.classList.add(row ? "nspanel-row" : "nspanel-tile");
    tile.classList.toggle("nspanel-bad", what.bad);
    tile.title = what.target;

    const icon = document.createElement("div");
    icon.classList.add("nspanel-tile-icon");
    icon.textContent = (typeof item === "string") ? "" : String(item?.icon ?? "");

    const name = document.createElement("div");
    name.classList.add("nspanel-tile-name");
    name.textContent = itemName(item);

    const type = document.createElement("div");
    type.classList.add("nspanel-tile-type");
    type.textContent = what.type;

    tile.append(icon, name, type);
    return tile;
}

/**
 * @param {string} title
 * @param {number} depth nested page depth: indented under its tile, "↳ Title"
 * @param {"grid" | "shortcuts" | "rows"} kind
 * @param {number} columns grid columns
 * @returns {{card: HTMLElement, head: HTMLElement, title: HTMLElement, grid: HTMLElement}}
 */
function previewCard(title, depth = 0, kind = "grid", columns = 4) {
    const card = document.createElement("div");
    card.classList.add("nspanel-card");
    card.classList.toggle("nspanel-nested", depth > 0);
    if (depth > 0) {
        card.style.marginLeft = `${depth * 24}px`;
    }

    const head = document.createElement("div");
    head.classList.add("nspanel-card-head");

    const name = document.createElement("div");
    name.classList.add("nspanel-card-title");
    name.textContent = (depth > 0) ? `↳ ${title}` : title;
    head.appendChild(name);

    const grid = document.createElement("div");
    grid.classList.add((kind === "rows") ? "nspanel-rows" : "nspanel-grid");
    grid.classList.toggle("nspanel-shortcuts", kind === "shortcuts");
    if ((kind === "grid") && (columns !== 4)) {
        grid.style.gridTemplateColumns = `repeat(${columns}, 1fr)`;
    }

    card.append(head, grid);
    return {card, head, title: name, grid};
}

// -----------------------------------------------------------------------------
// Editor: the preview is also a visual editor of the same JSON. Tiles, rows and page titles are
// dragged to another place, palette chips add items and pages, the trash removes them, a double click
// renames. Every change rewrites the text (pretty-printed) and keeps the previous text for Undo;
// nothing reaches the device before Save.
// -----------------------------------------------------------------------------

/** the configuration the preview shows, parsed from __text; the editor changes it in place */
let __config = /** @type {any} */ (null);
let __text = "";

/** @type {string[]} */
const __undo = [];
const UNDO_MAX = 50;

/**
 * what is dragged: an item of a list, a top-level page (by index), a palette template or a new page
 * @typedef {{kind: "item", list: () => any[], index: number} | {kind: "page", index: number}
 *     | {kind: "template", key: string} | {kind: "newpage"}} Drag
 */

/** @type {Drag | null} */
let __drag = null;

/** an item added from the palette: its name is edited once the preview is drawn again */
let __renameNext = /** @type {any} */ (null);

/**
 * the tile or page whose settings are shown; found again by identity after every redraw (the editor
 * changes __config in place), dropped when the text is parsed anew
 * @typedef {{kind: "item", item: any, list: () => any[], index: number} | {kind: "page", page: any}} Selection
 */

/** @type {Selection | null} */
let __selected = null;

/** the selection was drawn in the last renderConfig() */
let __selectedShown = false;

/**
 * the array under owner[key], created when missing
 * @param {any} owner
 * @param {string} key
 * @returns {any[]}
 */
function listOf(owner, key) {
    if (!Array.isArray(owner[key])) {
        owner[key] = [];
    }
    return owner[key];
}

/**
 * applies an editor change to __config and rewrites the text; the change returns false when it did nothing.
 * form: false keeps the settings form as it is (a change made in it, the focus stays in the next field)
 * @param {(config: any) => boolean | void} change
 * @param {boolean} form
 */
function commit(change, form = true) {
    const textarea = editor();
    if (!textarea || (__config === null)) {
        return;
    }

    if (textarea.value !== __text) {
        renderPreview(textarea.value);
        showResult("The text was edited since the preview was drawn, repeat the change", "error");
        return;
    }

    if (change(__config) === false) {
        renderConfig(__config, form);
        return;
    }

    __undo.push(__text);
    if (__undo.length > UNDO_MAX) {
        __undo.shift();
    }

    __text = JSON.stringify(__config, null, 2);
    textarea.value = __text;
    renderConfig(__config, form);
    showResult("Changed in the editor, Save to apply", "ok");
}

function onUndo() {
    const textarea = editor();
    const text = __undo.pop();
    if (!textarea || (text === undefined)) {
        showResult("Nothing to undo");
        return;
    }

    textarea.value = text;
    renderPreview(text);
    showResult("Undone, Save to apply", "ok");
}

function clearDropMarks() {
    document.querySelectorAll("#panel-nspanel .nspanel-drop")
        .forEach((elem) => {
            elem.classList.remove("nspanel-drop");
        });
}

/**
 * @param {HTMLElement} elem
 * @param {() => Drag} make
 */
function dragSource(elem, make) {
    elem.draggable = true;
    elem.addEventListener("dragstart", (event) => {
        event.stopPropagation();
        __drag = make();
        if (event.dataTransfer) {
            event.dataTransfer.setData("text/plain", "");
            event.dataTransfer.effectAllowed = "copyMove";
        }
        elem.classList.add("nspanel-dragging");
    });
    elem.addEventListener("dragend", () => {
        elem.classList.remove("nspanel-dragging");
        clearDropMarks();
        __drag = null;
    });
}

/**
 * @param {HTMLElement} elem
 * @param {(drag: Drag) => boolean} accepts
 * @param {(drag: Drag) => void} drop
 */
function dropZone(elem, accepts, drop) {
    elem.addEventListener("dragover", (event) => {
        if (__drag && accepts(__drag)) {
            event.preventDefault();
            event.stopPropagation();
            clearDropMarks();
            elem.classList.add("nspanel-drop");
        }
    });
    elem.addEventListener("dragleave", (event) => {
        if (!(event.relatedTarget instanceof Node) || !elem.contains(event.relatedTarget)) {
            elem.classList.remove("nspanel-drop");
        }
    });
    elem.addEventListener("drop", (event) => {
        const drag = __drag;
        if (!drag || !accepts(drag)) {
            return;
        }

        event.preventDefault();
        event.stopPropagation();
        clearDropMarks();
        __drag = null;
        drop(drag);
    });
}

/** @param {Drag} drag */
const isItem = (drag) => (drag.kind === "item") || (drag.kind === "template");

/** @param {Drag} drag */
const isPage = (drag) => (drag.kind === "page") || (drag.kind === "newpage");

/**
 * an item (moved or new) dropped at list()[index]
 * @param {Drag} drag
 * @param {() => any[]} list
 * @param {number} index
 */
function dropItem(drag, list, index) {
    commit(() => {
        const to = list();
        if (drag.kind === "item") {
            return moveItem(drag.list(), drag.index, to, index);
        }

        if (drag.kind === "template") {
            const item = TEMPLATES[drag.key].make();
            to.splice(Math.min(index, to.length), 0, item);
            __renameNext = item;
            return true;
        }

        return false;
    });
}

/**
 * a page (moved or new) dropped at config.pages[index]
 * @param {Drag} drag
 * @param {number} index
 */
function dropPage(drag, index) {
    commit((config) => {
        const pages = listOf(config, "pages");
        if (drag.kind === "page") {
            return moveItem(pages, drag.index, pages, index);
        }

        if (drag.kind === "newpage") {
            pages.splice(Math.min(index, pages.length), 0, newPage());
            return true;
        }

        return false;
    });
}

/**
 * replaces the text of elem with an input; Enter or leaving it applies, Escape cancels
 * @param {HTMLElement} elem
 * @param {string} current
 * @param {(text: string) => void} apply
 */
function startRename(elem, current, apply) {
    const owner = elem.closest("[draggable='true']");
    if (owner instanceof HTMLElement) {
        owner.draggable = false;
    }

    const input = document.createElement("input");
    input.classList.add("nspanel-rename");
    input.value = current;

    let done = false;
    /** @param {boolean} save */
    const finish = (save) => {
        if (done) {
            return;
        }
        done = true;

        const text = input.value.trim();
        if (save && (text !== current)) {
            commit(() => {
                apply(text);
            });
        } else {
            renderConfig(__config);
        }
    };

    input.addEventListener("keydown", (event) => {
        event.stopPropagation();
        if (event.key === "Enter") {
            event.preventDefault();
            finish(true);
        } else if (event.key === "Escape") {
            event.preventDefault();
            finish(false);
        }
    });
    input.addEventListener("blur", () => {
        finish(true);
    });

    elem.replaceChildren(input);
    input.focus();
    input.select();
}

/**
 * a tile or row of list()[index]: dragged elsewhere, takes drops, a double click renames it
 * @param {HTMLElement} tile
 * @param {() => any[]} list
 * @param {number} index
 */
function editableTile(tile, list, index) {
    tile.title = `${tile.title}${tile.title.length ? "\n" : ""}drag to move, double click to rename`;
    dragSource(tile, () => ({kind: "item", list, index}));
    dropZone(tile, isItem, (drag) => {
        dropItem(drag, list, index);
    });
    tile.addEventListener("click", () => {
        select(tile, {kind: "item", item: list()[index], list, index});
    });
    tile.addEventListener("dblclick", (event) => {
        const name = tile.querySelector(".nspanel-tile-name");
        if (!(name instanceof HTMLElement)) {
            return;
        }

        event.preventDefault();
        const item = list()[index];
        startRename(name, (typeof item === "string") ? item : String(item?.name ?? ""), (text) => {
            renameItem(list(), index, text);
        });
    });
}

/**
 * the tiles of a card; returns the tile of __renameNext when it is one of them
 * @param {HTMLElement} grid
 * @param {any[]} slots
 * @param {number[]} indexes per slot the index in list(), -1 for an empty slot
 * @param {() => any[]} list
 * @param {number} append where a drop on an empty slot or the card's free space inserts
 * @param {any} groups
 * @param {boolean} rows
 * @returns {HTMLElement[]}
 */
function editableTiles(grid, slots, indexes, list, append, groups, rows = false) {
    /** @type {HTMLElement[]} */
    const out = [];
    slots.forEach((item, slot) => {
        const tile = previewTile(item, groups, rows);
        if (indexes[slot] < 0) {
            dropZone(tile, isItem, (drag) => {
                dropItem(drag, list, append);
            });
        } else {
            editableTile(tile, list, indexes[slot]);
            if ((__renameNext !== null) && (item === __renameNext)) {
                out.push(tile);
            }
            if ((__selected?.kind === "item") && (item === __selected.item) && (typeof item === "object")) {
                __selected.list = list;
                __selected.index = indexes[slot];
                tile.classList.add("nspanel-selected");
                __selectedShown = true;
            }
        }
        grid.appendChild(tile);
    });

    dropZone(grid, isItem, (drag) => {
        dropItem(drag, list, append);
    });

    return out;
}

/**
 * the preview cards with the editor bound to them, and the settings form of the selection
 * @param {any} config
 * @param {boolean} form draw the settings form again
 */
function renderConfig(config, form = true) {
    const area = document.querySelector("#panel-nspanel .nspanel-preview-area");
    if (!(area instanceof HTMLElement) || (config === null) || (typeof config !== "object")) {
        return;
    }

    const groups = config?.groups;
    const cards = [];
    /** @type {HTMLElement[]} */
    let rename = [];
    __selectedShown = false;

    // home screen shortcuts: up to 7, empty slots take drops (the screensaver object is created then)
    {
        const shortcuts = Array.isArray(config?.screensaver?.shortcuts) ? config.screensaver.shortcuts : [];
        const shown = shortcuts.slice(0, 7);
        const slots = [...shown, ...Array(7 - shown.length).fill(null)];
        const indexes = slots.map((_, index) => (index < shown.length) ? index : -1);
        const list = () => {
            if ((typeof config.screensaver !== "object") || (config.screensaver === null)) {
                config.screensaver = {};
            }
            return listOf(config.screensaver, "shortcuts");
        };

        const {card, grid} = previewCard("Home: shortcuts", 0, "shortcuts");
        rename = rename.concat(editableTiles(grid, slots, indexes, list, shortcuts.length, groups));
        cards.push(card);
    }

    const pages = Array.isArray(config?.pages) ? config.pages : [];
    for (const preview of previewCards(config, PREVIEW_CAPACITY, true, __grids)) {
        const page = preview.page;
        const top = (preview.depth === 0) ? pages.indexOf(page) : -1;
        const settings = page?.type === "settings";
        const list = () => listOf(page, "items");
        const append = preview.offset + preview.items.length;

        let built;
        if (preview.list) {
            built = previewCard(preview.title, preview.depth, "rows");
            const indexes = preview.items.map((_, index) => preview.offset + index);
            rename = rename.concat(editableTiles(built.grid, preview.items, indexes, list, append, groups, true));
        } else if (settings) {
            built = previewCard(preview.title, preview.depth, "grid");
        } else {
            const {columns, slots, indexes} = previewGrid(preview, __grids);
            built = previewCard(`${preview.title} (${preview.grid})`, preview.depth, "grid", columns);
            const absolute = indexes.map((index) => (index < 0) ? -1 : (preview.offset + index));
            rename = rename.concat(editableTiles(built.grid, slots, absolute, list, append, groups));
        }

        const {card, head, title} = built;

        // the layout of a grid page
        if (!preview.list && !settings) {
            const select = document.createElement("select");
            select.title = "layout of this page";
            for (const grid of Object.keys(GRIDS)) {
                const option = document.createElement("option");
                option.value = grid;
                option.textContent = grid;
                option.selected = (grid === preview.grid);
                select.appendChild(option);
            }
            select.addEventListener("change", () => {
                commit(() => {
                    page.layout = select.value;
                    delete page.columns;
                });
            });
            head.appendChild(select);
        }

        // the title: double click renames the page, a top-level page is dragged by it
        title.title = (top >= 0) ? "drag to move the page, double click to rename" : "double click to rename";
        title.addEventListener("click", () => {
            select(title, {kind: "page", page});
        });
        if ((__selected?.kind === "page") && (__selected.page === page)) {
            title.classList.add("nspanel-selected");
            __selectedShown = true;
        }
        title.addEventListener("dblclick", (event) => {
            event.preventDefault();
            startRename(title, String(page?.title ?? ""), (text) => {
                if (text.length) {
                    page.title = text;
                } else {
                    delete page.title;
                }
            });
        });
        if (top >= 0) {
            dragSource(title, () => ({kind: "page", index: top}));
        }

        // the head takes pages (they go before this one) and items (appended to this page)
        dropZone(head, (drag) => (isPage(drag) && (top >= 0)) || (isItem(drag) && !settings), (drag) => {
            if (isPage(drag)) {
                dropPage(drag, top);
            } else {
                dropItem(drag, list, Number.MAX_SAFE_INTEGER);
            }
        });

        cards.push(card);
    }

    area.replaceChildren(...cards);

    if (!__selectedShown && (__selected !== null)) {
        __selected = null;
        form = true;
    }
    if (form) {
        renderForm();
    }

    if (rename.length) {
        __renameNext = null;
        rename[0].dispatchEvent(new MouseEvent("dblclick", {bubbles: true}));
    }
}

/**
 * a click on a tile or a page title shows its settings
 * @param {HTMLElement} elem
 * @param {Selection} selection
 */
function select(elem, selection) {
    document.querySelectorAll("#panel-nspanel .nspanel-selected")
        .forEach((other) => {
            other.classList.remove("nspanel-selected");
        });
    elem.classList.add("nspanel-selected");
    __selected = selection;
    renderForm();
}

/**
 * suggestions of a free text field
 * @param {string | undefined} name
 * @returns {string[]}
 */
function suggestions(name) {
    const config = __config ?? {};
    if (name === "groups") {
        return Object.keys(config.groups ?? {});
    }
    if (name === "pages") {
        const pages = Array.isArray(config.pages) ? config.pages : [];
        const out = pages.flatMap((page) => [page?.id, page?.title]).filter((value) => value);
        return [...out.map(String), "weather", "home", "settings"];
    }
    return [];
}

/**
 * one labelled field of the settings form; a change is written at once (the form stays as it is,
 * unless field.redraw)
 * @param {HTMLElement} form
 * @param {any} owner the object holding the field, or a function returning it
 * @param {Field} field
 */
function formField(form, owner, field) {
    const target = (typeof owner === "function") ? owner : () => owner;
    const row = document.createElement("label");
    row.classList.add("nspanel-field");

    const label = document.createElement("span");
    label.textContent = field.label;

    /** @type {HTMLInputElement | HTMLSelectElement | HTMLTextAreaElement} */
    let input;
    const current = target()?.[field.key];
    const value = ((field.type === "members") && Array.isArray(current)) ? current.join("\n") : formatValue(current);
    const choices = (field.choices === "icons") ? __icons : [];
    if (field.options || (field.type === "bool") || choices.length) {
        input = document.createElement("select");
        const values = field.options ?? (choices.length ? choices : ["true", "false"]);
        if (value.length && !values.includes(value)) {
            values.unshift(value);
        }
        for (const option of ["", ...values]) {
            const elem = document.createElement("option");
            elem.value = option;
            elem.textContent = option.length ? option : "(default)";
            input.appendChild(elem);
        }
        input.value = value;
    } else if (field.type === "members") {
        input = document.createElement("textarea");
        input.rows = 5;
        input.value = value;
    } else {
        input = document.createElement("input");
        input.type = "text";
        input.value = value;
        const proposals = suggestions(field.suggest);
        if (proposals.length) {
            const list = document.createElement("datalist");
            list.id = `nspanel-suggest-${field.key}`;
            for (const proposal of proposals) {
                const elem = document.createElement("option");
                elem.value = proposal;
                list.appendChild(elem);
            }
            row.appendChild(list);
            input.setAttribute("list", list.id);
        }
    }
    input.name = field.key;
    input.addEventListener("keydown", (event) => {
        if ((event.key === "Enter") && !(input instanceof HTMLTextAreaElement)) {
            event.preventDefault();
            input.dispatchEvent(new Event("change"));
        }
    });
    input.addEventListener("change", () => {
        commit(() => {
            applyField(target(), field.key, field.type ?? "text", input.value);
        }, !!field.redraw);
    });

    row.prepend(label);
    row.appendChild(input);
    form.appendChild(row);
}

/** the settings of the selected tile or page, next to the preview */
function renderForm() {
    const form = document.querySelector("#panel-nspanel .nspanel-form");
    if (!(form instanceof HTMLElement)) {
        return;
    }

    form.replaceChildren();
    const selected = __selected;
    if (selected === null) {
        return;
    }

    const head = document.createElement("div");
    head.classList.add("nspanel-form-head");
    const title = document.createElement("strong");
    const close = document.createElement("button");
    close.type = "button";
    close.classList.add("pure-button");
    close.textContent = "✕";
    close.title = "close";
    close.addEventListener("click", () => {
        __selected = null;
        renderConfig(__config);
    });
    head.append(title, close);
    form.appendChild(head);

    if (selected.kind === "page") {
        title.textContent = `Page: ${String(selected.page?.title ?? "")}`;
        for (const field of PAGE_FIELDS) {
            formField(form, selected.page, field);
        }
        return;
    }

    const item = selected.item;
    const kind = itemKind(item);
    title.textContent = `Tile: ${itemName(item)}`;

    if (kind === "ref") {
        const note = document.createElement("p");
        note.textContent = "A reference to the tile of this name, edit that tile.";
        form.appendChild(note);
        return;
    }

    // what it does: the fields of the kind follow
    const what = document.createElement("label");
    what.classList.add("nspanel-field");
    const label = document.createElement("span");
    label.textContent = "What it does";
    const kinds = document.createElement("select");
    for (const [key, text] of Object.entries(KINDS)) {
        const option = document.createElement("option");
        option.value = key;
        option.textContent = text;
        kinds.appendChild(option);
    }
    kinds.value = kind;
    kinds.addEventListener("change", () => {
        commit(() => {
            setItemKind(item, kinds.value);
        });
    });
    what.append(label, kinds);

    for (const field of COMMON_FIELDS) {
        formField(form, item, field);
    }
    form.appendChild(what);
    for (const field of (KIND_FIELDS[kind] ?? [])) {
        formField(form, field.nested ? item.page : item, field);
    }

    // a scene: the members of its group (top-level "groups", shared by every tile and button of the group)
    const group = String(item?.group ?? "");
    if ((kind === "scene") && group.length) {
        formField(form, () => {
            if ((typeof __config.groups !== "object") || (__config.groups === null) || Array.isArray(__config.groups)) {
                __config.groups = {};
            }
            return __config.groups;
        }, {
            key: group,
            label: `Members of "${group}": a state topic or a relay number a line (shared by its tiles)`,
            type: "members",
        });
    }

    const remove = document.createElement("button");
    remove.type = "button";
    remove.classList.add("pure-button", "nspanel-remove");
    remove.textContent = "Remove the tile";
    remove.addEventListener("click", () => {
        commit(() => {
            const list = selected.list();
            if (list[selected.index] !== item) {
                return false;
            }
            list.splice(selected.index, 1);
            __selected = null;
            return true;
        });
    });
    form.appendChild(remove);
}

/** @param {string} text */
function renderPreview(text) {
    let config;
    try {
        config = JSON.parse(text);
    } catch (_) {
        return; // keep the last good preview while typing
    }

    __config = config;
    __text = text;
    __selected = null;
    renderConfig(config);
}

/** the palette over the preview: templates to drag in, the trash, Undo */
function buildPalette() {
    const palette = document.querySelector("#panel-nspanel .nspanel-palette");
    if (!(palette instanceof HTMLElement)) {
        return;
    }

    /**
     * @param {string} label
     * @param {string} hint
     * @returns {HTMLElement}
     */
    const chip = (label, hint) => {
        const out = document.createElement("span");
        out.classList.add("nspanel-chip");
        out.textContent = label;
        out.title = hint;
        palette.appendChild(out);
        return out;
    };

    dragSource(chip("+ Page", "drag onto a page title (the new page goes before it) or below the last card"),
        () => ({kind: "newpage"}));
    for (const [key, template] of Object.entries(TEMPLATES)) {
        dragSource(chip(`+ ${template.label}`, "drag onto a tile, an empty slot or a page title"),
            () => ({kind: "template", key}));
    }

    const trash = chip("✕ Trash", "drop a tile or a page title here to remove it");
    trash.classList.add("nspanel-trash");
    trash.draggable = false;
    dropZone(trash, (drag) => (drag.kind === "item") || (drag.kind === "page"), (drag) => {
        commit((config) => {
            if (drag.kind === "item") {
                return drag.list().splice(drag.index, 1).length > 0;
            }
            if (drag.kind === "page") {
                return listOf(config, "pages").splice(drag.index, 1).length > 0;
            }
            return false;
        });
    });

    const undo = document.createElement("button");
    undo.type = "button";
    undo.classList.add("pure-button", "button-nspanel-undo");
    undo.textContent = "Undo";
    undo.addEventListener("click", onUndo);
    palette.appendChild(undo);

    // a new (or moved) page dropped below the cards goes last
    const area = document.querySelector("#panel-nspanel .nspanel-preview-area");
    if (area instanceof HTMLElement) {
        dropZone(area, isPage, (drag) => {
            dropPage(drag, Number.MAX_SAFE_INTEGER);
        });
    }
}

/** @type {ReturnType<typeof setTimeout> | null} */
let __previewTimer = null;

function onEditorInput() {
    if (__previewTimer !== null) {
        clearTimeout(__previewTimer);
    }

    __previewTimer = setTimeout(() => {
        __previewTimer = null;
        renderPreview(editor()?.value ?? "");
    }, 300);
}

/** @returns {HTMLTextAreaElement | null} */
function editor() {
    const out = document.getElementById("nspanel-config");
    return (out instanceof HTMLTextAreaElement) ? out : null;
}

/**
 * @param {string} text
 * @param {"ok" | "error" | ""} kind
 */
function showResult(text, kind = "") {
    const elem = document.querySelector("#panel-nspanel .nspanel-result");
    if (!(elem instanceof HTMLElement)) {
        return;
    }

    elem.textContent = text;
    elem.classList.toggle("nspanel-ok", kind === "ok");
    elem.classList.toggle("nspanel-error", kind === "error");
}

/** @param {boolean} value */
function setBusy(value) {
    document.querySelectorAll("#panel-nspanel button")
        .forEach((elem) => {
            if (elem instanceof HTMLButtonElement) {
                elem.disabled = value;
            }
        });
}

/**
 * @param {Response} response
 * @returns {Promise<string>}
 */
async function responseError(response) {
    const text = (await response.text()).trim();
    return `HTTP ${response.status}${text.length ? `: ${text}` : ` ${response.statusText}`}`;
}

/**
 * pretty-print when possible, device stores the file as uploaded
 * @param {string} text
 * @returns {string}
 */
function prettyJson(text) {
    try {
        return JSON.stringify(JSON.parse(text), null, 2);
    } catch (_) {
        return text;
    }
}

async function onLoad() {
    const textarea = editor();
    if (!textarea) {
        return;
    }

    setBusy(true);
    showResult("Loading...");

    try {
        const response = await fetch(endpoint("config"), FETCH_OPTIONS);
        if (!response.ok) {
            showResult(await responseError(response), "error");
            return;
        }

        textarea.value = prettyJson(await response.text());
        __loaded = true;
        __undo.length = 0;
        renderPreview(textarea.value);
        showResult(`Loaded ${textarea.value.length} characters`, "ok");
    } catch (e) {
        showResult(`Load failed: ${(e instanceof Error) ? e.message : String(e)}`, "error");
    } finally {
        setBusy(false);
    }
}

/** @returns {boolean} */
function onValidate() {
    const textarea = editor();
    if (!textarea) {
        return false;
    }

    const result = validateJson(textarea.value);
    showResult(formatValidateResult(result), result.ok ? "ok" : "error");

    if (!result.ok && (result.offset !== undefined)) {
        textarea.focus();
        textarea.setSelectionRange(result.offset, Math.min(textarea.value.length, result.offset + 1));
    }

    return result.ok;
}

async function onSave() {
    const textarea = editor();
    if (!textarea || !onValidate()) {
        return;
    }

    setBusy(true);
    showResult("Saving...");

    try {
        const response = await fetch(endpoint("config"), {
            ...FETCH_OPTIONS,
            method: "POST",
            headers: {"Content-Type": "application/json"},
            // without the indentation: the device takes at most 16 KB, a pretty-printed config is twice the size
            body: JSON.stringify(JSON.parse(textarea.value)),
        });

        const text = (await response.text()).trim();
        if (response.ok) {
            showResult(text.length ? text : "OK", "ok");
            await updateStatus();
        } else {
            showResult(`HTTP ${response.status}: ${text.length ? text : response.statusText}`, "error");
        }
    } catch (e) {
        showResult(`Save failed: ${(e instanceof Error) ? e.message : String(e)}`, "error");
    } finally {
        setBusy(false);
    }
}

/**
 * @param {any} status
 * @returns {string}
 */
export function formatStatus(status) {
    if (!status || (typeof status !== "object")) {
        return "?";
    }

    const parts = [String(status.driver ?? "?")];
    if (status.display && (status.display !== status.driver)) {
        parts.push(`(config: ${status.display})`);
    }

    if (status.model) {
        parts.push(`model ${status.model}`);
    }

    if (status.version) {
        parts.push(`version ${status.version}`);
    }

    if (status.design) {
        parts.push(String(status.design));
    }

    parts.push(status.connected ? "connected" : "not connected");

    if (status.probing) {
        parts.push("probing");
    }

    if (status.flashing) {
        parts.push("flashing TFT");
    }

    return parts.join(", ");
}

/**
 * which design the preview draws: full grids (custom pages or drawn tiles) unless the panel reports an
 * unknown OHpanel design (the layouts then use the 4x2 slots)
 * @param {any} status
 * @param {boolean} current
 * @returns {boolean}
 */
export function previewDesignGrids(status, current = true) {
    if ((status?.driver !== "ohpanel") || !status?.version || (status.version === "unknown")) {
        return current;
    }

    return DESIGNS_FULL_GRIDS.includes(status.version);
}

async function updateStatus() {
    const elem = document.querySelector("#panel-nspanel .nspanel-status");
    if (!(elem instanceof HTMLElement)) {
        return;
    }

    try {
        const response = await fetch(endpoint("status"), FETCH_OPTIONS);
        if (!response.ok) {
            elem.textContent = await responseError(response);
            return;
        }

        const status = await response.json();
        elem.textContent = formatStatus(status);
        if (Array.isArray(status?.icons)) {
            __icons = status.icons.map(String);
        }

        const grids = previewDesignGrids(status, __grids);
        if (grids !== __grids) {
            __grids = grids;
            renderPreview(editor()?.value ?? "");
        }
    } catch (e) {
        elem.textContent = `unavailable (${(e instanceof Error) ? e.message : String(e)})`;
    }
}

function onPanelShown() {
    updateStatus();
    if (!__loaded) {
        onLoad();
    }
}

/** @param {KeyboardEvent} event */
function onEditorKey(event) {
    const textarea = event.target;
    if (!(textarea instanceof HTMLTextAreaElement)) {
        return;
    }

    if ((event.key === "s") && (event.ctrlKey || event.metaKey)) {
        event.preventDefault();
        onSave();
        return;
    }

    if ((event.key !== "Tab") || event.shiftKey || event.ctrlKey || event.altKey || event.metaKey) {
        return;
    }

    event.preventDefault();

    const start = textarea.selectionStart;
    const end = textarea.selectionEnd;
    textarea.setRangeText("  ", start, end, "end");
}

export function init() {
    styleInject(STYLE);

    listenAppConnected((urls) => {
        __urls = {
            config: new URL(RELATIVE.config, urls.config),
            status: new URL(RELATIVE.status, urls.config),
        };
    });

    document.querySelector(".button-nspanel-load")
        ?.addEventListener("click", onLoad);
    document.querySelector(".button-nspanel-validate")
        ?.addEventListener("click", onValidate);
    document.querySelector(".button-nspanel-save")
        ?.addEventListener("click", onSave);

    buildPalette();

    editor()?.addEventListener("keydown", onEditorKey);
    editor()?.addEventListener("input", onEditorInput);

    document.querySelectorAll("[data-panel='nspanel']")
        .forEach((elem) => {
            elem.addEventListener("click", onPanelShown);
        });
}
