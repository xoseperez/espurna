// NSPanel screen mirror: a Nextion emulator for the OHpanel TFT design. The device copies every command it sends
// to the display ("nspanelTft", newline separated); they are drawn here on one 480x320 canvas the way the display
// draws them: a page load paints the background and the page's objects, an attribute change repaints one object
// (its box restored from the background first when it has none of its own), fill / cirs / line / xstr / picq paint
// over what is there. Touches go back as the display's own events. The design's geometry, pictures and fonts are
// OHpanel's (Apache-2.0), loaded from its repository at the commit the TFT was built from.

/** OHpanel commit of the stock TFT bc629e1e (v0.9.2) */
export const OHPANEL_COMMIT = "ba9361982399414d1612aea85504d9707b329e75";

export const DESIGN = "bc629e1e";

const ASSETS = `https://cdn.jsdelivr.net/gh/bruestel/ohpanel@${OHPANEL_COMMIT}/`;

export const WIDTH = 480;
export const HEIGHT = 320;

// a finger moving this far is a swipe (the TFT's own test, ohpanel-protocol.md 4.1)
const SWIPE = 100;

// display code point of an icon -> materialdesignicons-webfont code point (fonts.py MDI_SHIFT)
const MDI_SHIFT = 0xF0001 - 0xE000;

/**
 * the display's fonts (OHpanel boards/nspanel/nextion/fonts.py): cell height, then the TrueType font for text
 * and for icons (private use code points), each with its em size and baseline (px from the top of the cell)
 * @typedef {{family: string, em: number, base: number}} FontPart
 * @type {{height: number, text: FontPart | null, icon: FontPart | null}[]}
 */
export const FONTS = [
    {height: 16, text: {family: "nsp-ubuntu", em: 14.3, base: 13}, icon: null},
    {height: 18, text: {family: "nsp-ubuntu", em: 16.9, base: 15}, icon: null},
    {height: 22, text: {family: "nsp-ubuntu", em: 19.8, base: 18}, icon: null},
    {height: 26, text: {family: "nsp-ubuntu", em: 23.2, base: 21}, icon: null},
    {height: 36, text: {family: "nsp-ubuntu", em: 32.9, base: 30}, icon: null},
    {height: 48, text: {family: "nsp-ubuntu", em: 43.6, base: 40}, icon: {family: "nsp-mdi", em: 40, base: 40}},
    {height: 72, text: {family: "nsp-ubuntu", em: 64.9, base: 61}, icon: {family: "nsp-mdi", em: 60, base: 59}},
    {height: 24, text: {family: "nsp-noto", em: 17.5, base: 19}, icon: {family: "nsp-mdi", em: 19, base: 22}},
    {height: 32, text: {family: "nsp-noto", em: 23.2, base: 25}, icon: {family: "nsp-mdi", em: 27, base: 27}},
    {height: 48, text: null, icon: {family: "nsp-mdi", em: 40, base: 40}},
];

const FONT_FILES = {
    "nsp-ubuntu": "boards/nspanel/nextion/assets/fonts/Ubuntu-R.ttf",
    "nsp-noto": "boards/nspanel/nextion/assets/fonts/NotoSans-Regular.ttf",
    "nsp-mdi": "boards/nspanel/nextion/assets/fonts/materialdesignicons-webfont.ttf",
};

/**
 * @param {number} value RGB565
 * @returns {string}
 */
export function rgb565(value) {
    const v = Number(value) & 0xFFFF;
    const r = (v >> 11) & 0x1F;
    const g = (v >> 5) & 0x3F;
    const b = v & 0x1F;
    return `rgb(${(r << 3) | (r >> 2)},${(g << 2) | (g >> 4)},${(b << 3) | (b >> 2)})`;
}

/**
 * one 8-bit channel through 5 or 6 bits and back, as rgb565() expands them
 * @param {number} value
 * @param {number} bits
 * @returns {number}
 */
export function quantize(value, bits) {
    const v = value >> (8 - bits);
    return (v << (8 - bits)) | (v >> (2 * bits - 8));
}

/**
 * a picture in the display's RGB565 colours: its flat areas then match the solid colours of the objects
 * over it (a tile and the boxes of its icon and name)
 * @param {HTMLImageElement} image
 * @returns {HTMLCanvasElement}
 */
function quantized(image) {
    const canvas = document.createElement("canvas");
    canvas.width = image.naturalWidth;
    canvas.height = image.naturalHeight;
    const ctx = /** @type {CanvasRenderingContext2D} */ (canvas.getContext("2d"));
    ctx.drawImage(image, 0, 0);
    const data = ctx.getImageData(0, 0, canvas.width, canvas.height);
    const px = data.data;
    for (let n = 0; n < px.length; n += 4) {
        px[n] = quantize(px[n], 5);
        px[n + 1] = quantize(px[n + 1], 6);
        px[n + 2] = quantize(px[n + 2], 5);
    }
    ctx.putImageData(data, 0, 0);
    return canvas;
}

/**
 * the text of a Nextion string literal: \" and \\ escaped, \r a line break
 * @param {string} text
 * @returns {string}
 */
export function unescapeText(text) {
    let out = "";
    for (let n = 0; n < text.length; ++n) {
        const c = text[n];
        if ((c === "\\") && (n + 1 < text.length)) {
            const next = text[++n];
            out += (next === "r") ? "\n" : next;
        } else {
            out += c;
        }
    }
    return out;
}

/**
 * the comma separated arguments of a drawing command; the last one may be a quoted string
 * @param {string} text
 * @returns {(number | string)[]}
 */
export function splitArgs(text) {
    const out = [];
    let n = 0;
    while (n < text.length) {
        if (text[n] === "\"") {
            const end = text.lastIndexOf("\"");
            out.push(unescapeText(text.slice(n + 1, (end > n) ? end : text.length)));
            break;
        }
        let comma = text.indexOf(",", n);
        if (comma < 0) {
            comma = text.length;
        }
        out.push(Number(text.slice(n, comma)));
        n = comma + 1;
    }
    return out;
}

/**
 * one command, as the firmware writes it (without the FF FF FF terminator)
 * @typedef {{op: "page", page: number}
 *   | {op: "vis", name: string, on: boolean}
 *   | {op: "attr", name: string, attr: string, value: number | string}
 *   | {op: "var", name: string, value: number}
 *   | {op: "draw", name: string, args: (number | string)[]}
 *   | {op: "marker", name: string}
 *   | {op: "other"}} Command
 * @param {string} line
 * @returns {Command}
 */
export function parseCommand(line) {
    if (line.startsWith("#")) {
        return {op: "marker", name: line.slice(1).trim()};
    }

    const space = line.indexOf(" ");
    const head = (space < 0) ? line : line.slice(0, space);
    if (head === "page") {
        return {op: "page", page: Number(line.slice(space + 1))};
    }
    if (head === "vis") {
        const [name, on] = line.slice(space + 1).split(",");
        return {op: "vis", name, on: on?.trim() === "1"};
    }
    if (["fill", "cirs", "cir", "line", "xstr", "picq", "draw"].includes(head) && (space > 0)) {
        return {op: "draw", name: head, args: splitArgs(line.slice(space + 1))};
    }

    const eq = line.indexOf("=");
    if (eq > 0) {
        const left = line.slice(0, eq);
        const right = line.slice(eq + 1);
        const dot = left.indexOf(".");
        if (dot > 0) {
            const value = right.startsWith("\"")
                ? unescapeText(right.slice(1, right.endsWith("\"") ? -1 : undefined))
                : Number(right);
            return {op: "attr", name: left.slice(0, dot), attr: left.slice(dot + 1), value};
        }
        return {op: "var", name: left, value: Number(right)};
    }

    return {op: "other"};
}

/**
 * the swipe the TFT reports for a finger that came down at (x0, y0) and left at (x1, y1), or ""
 * @param {number} dx
 * @param {number} dy
 * @returns {"" | "left" | "right" | "up" | "down"}
 */
export function swipeOf(dx, dy) {
    if (dx > SWIPE) {
        return "right";
    }
    if (dx < -SWIPE) {
        return "left";
    }
    if (dy > SWIPE) {
        return "down";
    }
    if (dy < -SWIPE) {
        return "up";
    }
    return "";
}

/**
 * what a picture id shows, from the manifest's per-background tables: "background", a semantic picture
 * ("button3:on", "close:x", "rowcard2:box", "weather_home:rain"...) or undefined
 * @param {any} manifest
 * @returns {Map<number, string>}
 */
export function pictureNames(manifest) {
    const out = new Map();
    for (const bg of manifest?.backgrounds ?? []) {
        out.set(Number(bg.pic), "background");
    }
    for (const table of Object.values(manifest?.pictures ?? {})) {
        for (const [name, value] of Object.entries(/** @type {any} */ (table))) {
            if (typeof value === "number") {
                out.set(value, name);
                continue;
            }
            for (const [variant, id] of Object.entries(/** @type {any} */ (value ?? {}))) {
                out.set(Number(id), `${name}:${variant}`);
            }
        }
    }
    return out;
}

/**
 * the layout picture (a key of layout.pictures) that draws a semantic picture, else a box colour or nothing
 * @param {string} name
 * @returns {{picture?: string, box?: number, badge?: string}}
 */
export function pictureFor(name) {
    const [what, variant] = name.split(":");
    if (/^button\d$/.test(what)) {
        return {picture: (variant === "on") ? "tile:on" : "tile:off"};
    }
    if (/^side\d$/.test(what)) {
        return {picture: (variant === "on") ? "tile:side_on" : "tile:side_off"};
    }
    if (what === "close") {
        return {picture: "close:x"};
    }
    if ((what === "weather_home") || (what === "weather_page")) {
        return {picture: `weather:${variant}`};
    }
    if (/^(card|rowcard|wchip)\d$/.test(what) || (what === "fav") || (what === "vol")) {
        return {box: 10597};
    }
    if (/^badge\d$/.test(what)) {
        return {badge: variant};
    }
    return {};
}

// What the TFT does itself when a page loads (OHpanel ohpanel_hmi.py: on_load, THEME_COLOURS,
// hidden_until_drawn): most pages hide everything and show the title and the close cross, the firmware
// shows the rest; a few keep their objects and hide only the pictures the firmware sets
const KEEP_OBJECTS = ["settings", "boot", "blank", "volpage", "qr", "system", "netpage"];
const SHOWN_AT_LOAD = /** @type {{[k: string]: string[]}} */ ({
    home: [],
    buttons: ["close_hs"],
});
const SHOWN_DEFAULT = ["title", "close_hs", "close"];

/**
 * the theme variable a page gives an object's pco when it loads, or ""
 * @param {string} page
 * @param {string} name
 * @returns {string}
 */
export function themeColour(page, name) {
    if (page === "home") {
        if (/^(date|info[12]_right|row[12]_icon|low_arrow|high_arrow\d+|high\d+|low\d+|outdoor_u\d+|row[12]_u\d+)$/.test(name)) {
            return "th_dim";
        }
        if (/^info[12]_(tag|text)$/.test(name)) {
            return "th_text";
        }
    } else if (page === "weather") {
        if (/^(low|low_arrow|low\d+|low_arrow\d+)$/.test(name)) {
            return "th_dim";
        }
    } else if (page === "qr") {
        if (/^caption[12]$/.test(name)) {
            return "th_text";
        }
    } else if ((page === "settings") && (name === "title")) {
        return "th_text";
    }
    if (/^busy\d$/.test(name)) {
        return "th_dim";
    }
    return "";
}

/**
 * whether an object shows right after its page loaded, before the firmware's commands
 * @param {string} page
 * @param {{n: string, t: number}} o
 * @param {boolean} composite its picture is one the firmware sets for the background in use
 * @returns {boolean}
 */
export function shownAtLoad(page, o, composite) {
    if (/^busy\d$/.test(o.n)) {
        return false;
    }
    if (KEEP_OBJECTS.includes(page)) {
        return !(composite && (o.t === 112) && (o.n !== "close"));
    }
    return (SHOWN_AT_LOAD[page] ?? SHOWN_DEFAULT).includes(o.n);
}

const BADGES = /** @type {{[k: string]: string}} */ ({
    "black": "#000000", "blue": "#2196f3", "brown": "#795548", "dark-grey": "#424242", "green": "#4caf50",
    "grey": "#9e9e9e", "light-grey": "#d0d0d0", "red": "#f44336", "white": "#ffffff", "yellow": "#ffeb3b",
});

/**
 * @typedef {{n: string, t: number, x: number, y: number, w: number, h: number, pic?: number, f?: number,
 *   pco?: number, bco?: number, sta?: number, xc?: number, yc?: number, txt?: string, val?: number,
 *   vis?: boolean}} ScreenObject
 */

export class Screen {
    /**
     * @param {HTMLCanvasElement} canvas
     * @param {(action: string, data: any) => void} send
     */
    constructor(canvas, send) {
        this.canvas = canvas;
        this.ctx = /** @type {CanvasRenderingContext2D} */ (canvas.getContext("2d"));
        this.send = send;

        /** @type {any} */
        this.layout = null;
        /** @type {any} */
        this.manifest = null;
        /** @type {Map<number, string>} */
        this.pictures = new Map();
        /** @type {Map<string, HTMLCanvasElement>} */
        this.images = new Map();
        /** @type {{[k: number]: string}} */
        this.pageNames = {};

        this.ready = false;
        /** @type {string[]} */
        this.pending = [];

        this.page = "";
        /** @type {ScreenObject[]} */
        this.objects = [];
        /** @type {Map<string, ScreenObject>} */
        this.byName = new Map();
        /** @type {Map<string, boolean>} touch areas without an object of their own: shown or not */
        this.areas = new Map();
        /** @type {{[k: string]: number}} */
        this.vars = {bg_pic: 48, bg_close: 45, sendxy: 0, dim: 100};

        /** @type {{x: number, y: number, hotspots: any[]} | null} */
        this.touch = null;

        canvas.width = WIDTH;
        canvas.height = HEIGHT;
        canvas.addEventListener("pointerdown", (event) => this.onDown(event));
        canvas.addEventListener("pointerup", (event) => this.onUp(event));
        canvas.addEventListener("pointercancel", () => { this.touch = null; });
    }

    /** loads the design's geometry, pictures and fonts once */
    async load() {
        const [layout, manifest] = await Promise.all([
            fetch(`${ASSETS}www/layout.json`).then((r) => r.json()),
            fetch(`${ASSETS}boards/nspanel/nextion/display-manifest.json`).then((r) => r.json()),
        ]);

        this.layout = layout;
        this.manifest = manifest;
        this.pictures = pictureNames(manifest);
        for (const [name, page] of Object.entries(manifest.pages ?? {})) {
            this.pageNames[Number(/** @type {any} */ (page).id)] = name;
        }

        const images = Object.entries(layout.pictures ?? {}).map(([key, url]) => new Promise((resolve) => {
            const image = new Image();
            image.onload = () => {
                this.images.set(key, quantized(image));
                resolve(null);
            };
            image.onerror = () => resolve(null);
            image.src = String(url);
        }));

        const fonts = Object.entries(FONT_FILES).map(async ([family, path]) => {
            const face = new FontFace(family, `url(${ASSETS}${path})`);
            document.fonts.add(await face.load());
        });

        await Promise.all([...images, ...fonts]);

        this.ready = true;
        const lines = this.pending;
        this.pending = [];
        this.feed(lines.join("\n"));
    }

    /**
     * commands from the device, newline separated
     * @param {string} text
     */
    feed(text) {
        const lines = text.split("\n").filter((line) => line.length);
        if (!this.ready) {
            this.pending.push(...lines);
            return;
        }

        for (const line of lines) {
            this.run(parseCommand(line));
        }
    }

    /** @param {Command} command */
    run(command) {
        switch (command.op) {
        case "marker":
            if (command.name !== "reset") {
                this.message(command.name === "offline" ? "The display does not answer" : "No OHpanel display");
            }
            return;
        case "page":
            this.loadPage(command.page);
            return;
        case "vis":
            this.setVisible(command.name, command.on);
            return;
        case "attr":
            this.setAttr(command.name, command.attr, command.value);
            return;
        case "var":
            this.vars[command.name] = command.value;
            if (command.name === "dim") {
                this.canvas.style.filter = (command.value > 0)
                    ? `brightness(${0.35 + 0.65 * Math.min(100, command.value) / 100})` : "brightness(0.05)";
            }
            return;
        case "draw":
            this.draw(command.name, command.args);
            return;
        }
    }

    /** @param {string} text */
    message(text) {
        const ctx = this.ctx;
        ctx.fillStyle = "#000";
        ctx.fillRect(0, 0, WIDTH, HEIGHT);
        ctx.fillStyle = "#ccc";
        ctx.font = "18px sans-serif";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText(text, WIDTH / 2, HEIGHT / 2);
        ctx.textAlign = "left";
        ctx.textBaseline = "alphabetic";
    }

    /** @param {number} id */
    loadPage(id) {
        const name = this.pageNames[id] ?? "";
        this.page = name;
        const page = this.layout?.pages?.[name];

        this.objects = (page?.objects ?? []).map((/** @type {any} */ o) => {
            const out = {...o, vis: shownAtLoad(name, o, (o.t === 112) && this.pictures.has(Number(o.pic)))};
            const theme = themeColour(name, o.n);
            if (theme && (this.vars[theme] !== undefined)) {
                out.pco = this.vars[theme];
            }
            return out;
        });
        this.byName = new Map(this.objects.map((o) => [o.n, o]));
        // touch areas that are not among the drawn objects (the tiles' hotspots) are shown and hidden all the same:
        // on a drawn grid the stock tiles' areas stay hidden and the touch goes by its coordinates
        this.areas = new Map();
        for (const t of Array.isArray(page?.touch) ? page.touch : []) {
            if (!this.byName.has(t.n)) {
                this.areas.set(t.n, shownAtLoad(name, {n: t.n, t: 109}, false));
            }
        }
        // the TFT shows its close cross from bg_close
        const close = this.byName.get("close");
        if (close && (close.t === 112)) {
            close.pic = this.vars.bg_close;
        }

        this.paintBackground(0, 0, WIDTH, HEIGHT);
        for (const o of this.objects) {
            this.paintObject(o, false);
        }
    }

    /**
     * the page's background picture over a box (black on pages without one)
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     */
    paintBackground(x, y, w, h) {
        const ctx = this.ctx;
        const page = this.manifest?.pages?.[this.page];
        const image = page?.background ? this.images.get(String(this.vars.bg_pic)) : undefined;
        if (image) {
            ctx.drawImage(image, x, y, w, h, x, y, w, h);
        } else {
            ctx.fillStyle = "#000";
            ctx.fillRect(x, y, w, h);
        }
    }

    /**
     * @param {string} name
     * @param {boolean} on
     */
    setVisible(name, on) {
        if (this.areas.has(name)) {
            this.areas.set(name, on);
            return;
        }

        if (name === "255") {
            for (const o of this.objects) {
                o.vis = on;
            }
            for (const area of this.areas.keys()) {
                this.areas.set(area, on);
            }
            if (on) {
                this.objects.forEach((o) => this.paintObject(o));
            } else {
                this.paintBackground(0, 0, WIDTH, HEIGHT);
            }
            return;
        }

        const o = this.byName.get(name);
        if (!o || (o.vis === on)) {
            return;
        }
        o.vis = on;
        if (on) {
            this.paintObject(o);
        } else if (o.t !== 109) {
            this.paintBackground(o.x, o.y, o.w, o.h);
        }
    }

    /**
     * @param {string} name
     * @param {string} attr
     * @param {number | string} value
     */
    setAttr(name, attr, value) {
        const o = /** @type {any} */ (this.byName.get(name));
        if (!o) {
            return;
        }
        o[attr] = value;
        if (o.vis) {
            this.paintObject(o);
        }
    }

    /**
     * @param {ScreenObject} o
     * @param {boolean} restore the background under the box first
     */
    paintObject(o, restore = true) {
        if (!o.vis) {
            return;
        }

        switch (o.t) {
        case 112:
            this.paintPicture(o.pic ?? 65535, o.x, o.y, o.w, o.h);
            return;
        case 116:
        case 55:
        case 98:
            if ((o.sta ?? 0) === 1) {
                this.ctx.fillStyle = rgb565(o.bco ?? 0);
                this.ctx.fillRect(o.x, o.y, o.w, o.h);
            } else if (restore) {
                this.paintBackground(o.x, o.y, o.w, o.h);
            }
            this.paintText(o.x, o.y, o.w, o.h, o.f ?? 0, o.pco ?? 65535, o.xc ?? 0, o.yc ?? 0, String(o.txt ?? ""));
            return;
        case 106:
        case 1: {
            const level = Math.max(0, Math.min(100, Number(o.val ?? 0))) / 100;
            this.ctx.fillStyle = rgb565(o.bco ?? 0);
            this.ctx.fillRect(o.x, o.y, o.w, o.h);
            this.ctx.fillStyle = rgb565(o.pco ?? 0);
            if (o.h > o.w) {
                this.ctx.fillRect(o.x, o.y + o.h * (1 - level), o.w, o.h * level);
            } else {
                this.ctx.fillRect(o.x, o.y, o.w * level, o.h);
            }
            return;
        }
        }
    }

    /**
     * a picture by its id: the background, a tile / card / icon of the design, else the background under it
     * @param {number} id
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     */
    paintPicture(id, x, y, w, h) {
        const name = this.pictures.get(Number(id));
        this.paintBackground(x, y, w, h);
        if (!name) {
            return;
        }

        const ctx = this.ctx;
        if (name === "background") {
            const image = this.images.get(String(id));
            if (image) {
                ctx.drawImage(image, x, y, w, h, x, y, w, h);
            }
            return;
        }

        const what = pictureFor(name);
        if (what.picture) {
            const image = this.images.get(what.picture);
            if (image) {
                ctx.drawImage(image, x, y, w, h);
            }
            return;
        }

        const colour = (what.box !== undefined) ? rgb565(what.box) : (what.badge ? BADGES[what.badge] : "");
        if (colour) {
            ctx.fillStyle = colour;
            ctx.beginPath();
            ctx.roundRect(x, y, w, h, Math.min(10, h / 3));
            ctx.fill();
        }
    }

    /**
     * text in a box the way the display draws it: one font cell high per line, aligned by xcen / ycen
     * (0 left / top, 1 centre, 2 right / bottom), cut off at the box
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} font
     * @param {number} pco
     * @param {number} xcen
     * @param {number} ycen
     * @param {string} text
     */
    paintText(x, y, w, h, font, pco, xcen, ycen, text) {
        if (!text.length) {
            return;
        }

        const spec = FONTS[font] ?? FONTS[0];
        const ctx = this.ctx;
        const lines = text.split("\n");
        const block = lines.length * spec.height;
        let top = y;
        if (ycen === 1) {
            top = y + Math.round((h - block) / 2);
        } else if (ycen === 2) {
            top = y + h - block;
        }

        ctx.save();
        ctx.beginPath();
        ctx.rect(x, y, w, h);
        ctx.clip();
        ctx.fillStyle = rgb565(pco);
        ctx.textBaseline = "alphabetic";

        lines.forEach((line, index) => {
            const runs = this.runs(line, spec);
            const width = runs.reduce((sum, run) => sum + run.width, 0);
            let pen = x;
            if (xcen === 1) {
                pen = x + Math.round((w - width) / 2);
            } else if (xcen === 2) {
                pen = x + w - width;
            }
            const cell = top + index * spec.height;
            for (const run of runs) {
                ctx.font = `${run.part.em}px ${run.part.family}`;
                ctx.fillText(run.text, pen, cell + run.part.base);
                pen += run.width;
            }
        });

        ctx.restore();
    }

    /**
     * a line split into runs of one font part (icons: code points from U+E000, drawn with MDI)
     * @param {string} line
     * @param {typeof FONTS[0]} spec
     * @returns {{text: string, part: FontPart, width: number}[]}
     */
    runs(line, spec) {
        /** @type {{text: string, part: FontPart, width: number}[]} */
        const out = [];
        for (const char of line) {
            const code = /** @type {number} */ (char.codePointAt(0));
            // the display's icon code points start at U+E000 and run past the private use area
            const icon = (code >= 0xE000) && (code < 0x10000) && (spec.icon !== null);
            const part = icon ? (spec.icon ?? spec.text) : (spec.text ?? spec.icon);
            if (!part) {
                continue;
            }
            const glyph = (icon && (part.family === "nsp-mdi")) ? String.fromCodePoint(code + MDI_SHIFT) : char;
            const last = out[out.length - 1];
            if (last && (last.part === part)) {
                last.text += glyph;
            } else {
                out.push({text: glyph, part, width: 0});
            }
        }
        for (const run of out) {
            this.ctx.font = `${run.part.em}px ${run.part.family}`;
            run.width = this.ctx.measureText(run.text).width;
        }
        return out;
    }

    /**
     * @param {string} name
     * @param {(number | string)[]} args
     */
    draw(name, args) {
        const ctx = this.ctx;
        const n = (/** @type {number} */ index) => Number(args[index] ?? 0);
        switch (name) {
        case "fill":
            ctx.fillStyle = rgb565(n(4));
            ctx.fillRect(n(0), n(1), n(2), n(3));
            return;
        case "cirs":
        case "cir":
            ctx.beginPath();
            ctx.arc(n(0), n(1), n(2), 0, 2 * Math.PI);
            if (name === "cirs") {
                ctx.fillStyle = rgb565(n(3));
                ctx.fill();
            } else {
                ctx.strokeStyle = rgb565(n(3));
                ctx.stroke();
            }
            return;
        case "line":
            ctx.beginPath();
            ctx.moveTo(n(0) + 0.5, n(1) + 0.5);
            ctx.lineTo(n(2) + 0.5, n(3) + 0.5);
            ctx.strokeStyle = rgb565(n(4));
            ctx.lineWidth = 1;
            ctx.stroke();
            return;
        case "picq":
            this.paintCrop(n(0), n(1), n(2), n(3), n(4));
            return;
        case "xstr": {
            // xstr x,y,w,h,font,pco,bco,xcen,ycen,sta,"text": sta 1 fills the box with bco first, 3 draws over
            if (n(9) === 1) {
                ctx.fillStyle = rgb565(n(6));
                ctx.fillRect(n(0), n(1), n(2), n(3));
            } else if (n(9) === 0) {
                this.paintBackground(n(0), n(1), n(2), n(3));
            }
            this.paintText(n(0), n(1), n(2), n(3), n(4), n(5), n(7), n(8), String(args[10] ?? ""));
            return;
        }
        }
    }

    /**
     * picq: the part of a full-screen picture over a box (the firmware restores the background with it)
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} id
     */
    paintCrop(x, y, w, h, id) {
        const image = this.images.get(String(id));
        if (image) {
            this.ctx.drawImage(image, x, y, w, h, x, y, w, h);
        } else {
            this.paintBackground(x, y, w, h);
        }
    }

    // ---------------------------------------------------------------------------------------------------------
    // Touch: hotspots of the page (their component's press / release), swipes, sendxy coordinates
    // ---------------------------------------------------------------------------------------------------------

    /**
     * @param {PointerEvent} event
     * @returns {{x: number, y: number}}
     */
    point(event) {
        const rect = this.canvas.getBoundingClientRect();
        return {
            x: Math.round((event.clientX - rect.left) * WIDTH / rect.width),
            y: Math.round((event.clientY - rect.top) * HEIGHT / rect.height),
        };
    }

    /**
     * the visible hotspots under a point, with their events
     * @param {number} x
     * @param {number} y
     * @returns {any[]}
     */
    hotspots(x, y) {
        const touch = this.layout?.pages?.[this.page]?.touch;
        if (!Array.isArray(touch)) {
            return [];
        }
        return touch.filter((t) => (x >= t.x) && (x < t.x + t.w) && (y >= t.y) && (y < t.y + t.h)
            && (this.byName.get(t.n)?.vis ?? this.areas.get(t.n) ?? false));
    }

    /**
     * @param {string} component
     * @param {string} value
     */
    event(component, value) {
        this.send("nspanelTouch", {page: this.page || "home", component, value});
    }

    /** @param {PointerEvent} event */
    onDown(event) {
        if (!this.ready) {
            return;
        }
        event.preventDefault();
        this.canvas.setPointerCapture(event.pointerId);

        const {x, y} = this.point(event);
        const hotspots = this.hotspots(x, y);
        this.touch = {x, y, hotspots};

        if (this.vars.sendxy) {
            this.send("nspanelXY", {x, y, press: 1});
        }
        for (const component of new Set(hotspots.filter((t) => t.v === "press").map((t) => t.c))) {
            this.event(component, "press");
        }
    }

    /** @param {PointerEvent} event */
    onUp(event) {
        const touch = this.touch;
        this.touch = null;
        if (!touch) {
            return;
        }

        const {x, y} = this.point(event);
        if (this.page === "blank") {
            this.event("wake", "release");
            return;
        }

        if (this.vars.sendxy) {
            this.send("nspanelXY", {x, y, press: 0});
        }

        const swipe = swipeOf(x - touch.x, y - touch.y);
        if (swipe && (this.manifest?.objects?.[this.page] ?? []).includes("swipe")) {
            this.event("swipe", swipe);
        }

        // the component the finger came down on, as the display reports it (also after a swipe: the firmware
        // drops a release that follows one)
        const released = new Set(touch.hotspots.filter((t) => t.v === "release").map((t) => t.c));
        for (const component of released) {
            this.event(component, "release");
        }
    }
}
