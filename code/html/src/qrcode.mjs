/**
 * Small QR Code encoder: byte mode, error correction level M, versions 1 to 6 (up to 106 bytes).
 * Enough for setup payloads like the Matter "MT:..." string, drawn on a canvas.
 * Structure follows ISO/IEC 18004: Reed-Solomon over GF(256) with 0x11D, the mask with the lowest penalty.
 */

/** version: [EC codewords per block, blocks, data codewords per block] */
const ECC_M = {
    1: [10, 1, 16],
    2: [16, 1, 28],
    3: [26, 1, 44],
    4: [18, 2, 32],
    5: [24, 2, 43],
    6: [16, 4, 27],
};

/** centers of the alignment patterns */
const ALIGNMENT = {
    1: [],
    2: [6, 18],
    3: [6, 22],
    4: [6, 26],
    5: [6, 30],
    6: [6, 34],
};

/**
 * @param {number} x
 * @param {number} y
 * @returns {number}
 */
function gfMultiply(x, y) {
    let z = 0;
    for (let i = 7; i >= 0; i--) {
        z = (z << 1) ^ ((z >>> 7) * 0x11D);
        z ^= ((y >>> i) & 1) * x;
    }

    return z & 0xFF;
}

/**
 * @param {number} degree
 * @returns {number[]}
 */
function rsDivisor(degree) {
    const out = new Array(degree).fill(0);
    out[degree - 1] = 1;

    let root = 1;
    for (let i = 0; i < degree; i++) {
        for (let j = 0; j < out.length; j++) {
            out[j] = gfMultiply(out[j], root);
            if (j + 1 < out.length) {
                out[j] ^= out[j + 1];
            }
        }
        root = gfMultiply(root, 0x02);
    }

    return out;
}

/**
 * @param {number[]} data
 * @param {number[]} divisor
 * @returns {number[]}
 */
function rsRemainder(data, divisor) {
    const out = divisor.map(() => 0);
    for (const value of data) {
        const factor = value ^ /** @type {number} */(out.shift());
        out.push(0);
        divisor.forEach((coefficient, index) => {
            out[index] ^= gfMultiply(coefficient, factor);
        });
    }

    return out;
}

/**
 * @param {number} mask
 * @param {number} x
 * @param {number} y
 * @returns {boolean}
 */
function masked(mask, x, y) {
    switch (mask) {
    case 0: return (x + y) % 2 === 0;
    case 1: return y % 2 === 0;
    case 2: return x % 3 === 0;
    case 3: return (x + y) % 3 === 0;
    case 4: return (Math.floor(x / 3) + Math.floor(y / 2)) % 2 === 0;
    case 5: return ((x * y) % 2) + ((x * y) % 3) === 0;
    case 6: return (((x * y) % 2) + ((x * y) % 3)) % 2 === 0;
    }

    return (((x + y) % 2) + ((x * y) % 3)) % 2 === 0;
}

/**
 * @param {boolean[][]} modules
 * @returns {number}
 */
function penalty(modules) {
    const size = modules.length;
    let out = 0;

    // runs of 5 or more modules of the same color, in rows and in columns
    for (let a = 0; a < size; a++) {
        let row = 1;
        let column = 1;
        for (let b = 1; b < size; b++) {
            row = (modules[a][b] === modules[a][b - 1]) ? row + 1 : 1;
            if (row === 5) {
                out += 3;
            } else if (row > 5) {
                out += 1;
            }

            column = (modules[b][a] === modules[b - 1][a]) ? column + 1 : 1;
            if (column === 5) {
                out += 3;
            } else if (column > 5) {
                out += 1;
            }
        }
    }

    // 2x2 blocks of the same color
    for (let y = 0; y < size - 1; y++) {
        for (let x = 0; x < size - 1; x++) {
            const color = modules[y][x];
            if ((color === modules[y][x + 1]) && (color === modules[y + 1][x]) && (color === modules[y + 1][x + 1])) {
                out += 3;
            }
        }
    }

    // balance of dark and light modules
    let dark = 0;
    for (const row of modules) {
        for (const module of row) {
            if (module) {
                dark++;
            }
        }
    }

    const total = size * size;
    out += 10 * Math.floor(Math.abs((dark * 20) - (total * 10)) / total);

    return out;
}

/**
 * @param {string} text
 * @returns {boolean[][]} rows of modules, true is dark
 */
export function qrEncode(text) {
    const bytes = Array.from(new TextEncoder().encode(text));

    let version = 0;
    for (let candidate = 1; candidate <= 6; candidate++) {
        const [, blocks, dataLength] = ECC_M[candidate];
        if ((4 + 8 + (bytes.length * 8)) <= (blocks * dataLength * 8)) {
            version = candidate;
            break;
        }
    }

    if (!version) {
        throw new Error("Text is too long for a version 6 QR code");
    }

    const [ecLength, blocks, dataLength] = ECC_M[version];
    const capacity = blocks * dataLength * 8;

    /** @type {number[]} */
    const bits = [];

    /**
     * @param {number} value
     * @param {number} length
     */
    const push = (value, length) => {
        for (let i = length - 1; i >= 0; i--) {
            bits.push((value >>> i) & 1);
        }
    };

    push(0b0100, 4);
    push(bytes.length, 8);
    bytes.forEach((value) => push(value, 8));
    push(0, Math.min(4, capacity - bits.length));
    push(0, (8 - (bits.length % 8)) % 8);
    for (let pad = 0xEC; bits.length < capacity; pad ^= 0xEC ^ 0x11) {
        push(pad, 8);
    }

    /** @type {number[]} */
    const data = [];
    for (let i = 0; i < bits.length; i += 8) {
        data.push(bits.slice(i, i + 8).reduce((out, bit) => (out << 1) | bit, 0));
    }

    const divisor = rsDivisor(ecLength);
    const dataBlocks = [];
    const ecBlocks = [];
    for (let block = 0; block < blocks; block++) {
        const part = data.slice(block * dataLength, (block + 1) * dataLength);
        dataBlocks.push(part);
        ecBlocks.push(rsRemainder(part, divisor));
    }

    /** @type {number[]} */
    const codewords = [];
    for (let i = 0; i < dataLength; i++) {
        dataBlocks.forEach((block) => codewords.push(block[i]));
    }
    for (let i = 0; i < ecLength; i++) {
        ecBlocks.forEach((block) => codewords.push(block[i]));
    }

    const size = 17 + (4 * version);

    /** @type {boolean[][]} */
    const modules = Array.from({length: size}, () => new Array(size).fill(false));
    /** @type {boolean[][]} */
    const reserved = Array.from({length: size}, () => new Array(size).fill(false));

    /**
     * @param {number} x
     * @param {number} y
     * @param {boolean} dark
     */
    const set = (x, y, dark) => {
        modules[y][x] = dark;
        reserved[y][x] = true;
    };

    for (let i = 0; i < size; i++) {
        set(6, i, (i % 2) === 0);
        set(i, 6, (i % 2) === 0);
    }

    /**
     * finder pattern with its separator
     * @param {number} cx
     * @param {number} cy
     */
    const finder = (cx, cy) => {
        for (let dy = -4; dy <= 4; dy++) {
            for (let dx = -4; dx <= 4; dx++) {
                const x = cx + dx;
                const y = cy + dy;
                if ((x >= 0) && (x < size) && (y >= 0) && (y < size)) {
                    const distance = Math.max(Math.abs(dx), Math.abs(dy));
                    set(x, y, (distance !== 2) && (distance !== 4));
                }
            }
        }
    };

    finder(3, 3);
    finder(size - 4, 3);
    finder(3, size - 4);

    const centers = ALIGNMENT[version];
    const last = centers.length - 1;
    for (let i = 0; i < centers.length; i++) {
        for (let j = 0; j < centers.length; j++) {
            if (((i === 0) && (j === 0)) || ((i === 0) && (j === last)) || ((i === last) && (j === 0))) {
                continue;
            }

            for (let dy = -2; dy <= 2; dy++) {
                for (let dx = -2; dx <= 2; dx++) {
                    set(centers[i] + dx, centers[j] + dy, Math.max(Math.abs(dx), Math.abs(dy)) !== 1);
                }
            }
        }
    }

    /**
     * format information: level M (00) and the mask, BCH(15,5), both copies, plus the dark module
     * @param {number} mask
     */
    const format = (mask) => {
        const value = mask;
        let remainder = value;
        for (let i = 0; i < 10; i++) {
            remainder = (remainder << 1) ^ ((remainder >>> 9) * 0x537);
        }

        const word = ((value << 10) | remainder) ^ 0x5412;
        /** @param {number} index */
        const bit = (index) => ((word >>> index) & 1) !== 0;

        for (let i = 0; i <= 5; i++) {
            set(8, i, bit(i));
        }
        set(8, 7, bit(6));
        set(8, 8, bit(7));
        set(7, 8, bit(8));
        for (let i = 9; i < 15; i++) {
            set(14 - i, 8, bit(i));
        }

        for (let i = 0; i < 8; i++) {
            set(size - 1 - i, 8, bit(i));
        }
        for (let i = 8; i < 15; i++) {
            set(8, size - 15 + i, bit(i));
        }

        set(8, size - 8, true);
    };

    format(0);

    let index = 0;
    const total = codewords.length * 8;
    for (let right = size - 1; right >= 1; right -= 2) {
        if (right === 6) {
            right = 5;
        }

        for (let vertical = 0; vertical < size; vertical++) {
            for (let j = 0; j < 2; j++) {
                const x = right - j;
                const upward = ((right + 1) & 2) === 0;
                const y = upward ? (size - 1 - vertical) : vertical;
                if (!reserved[y][x] && (index < total)) {
                    modules[y][x] = ((codewords[index >>> 3] >>> (7 - (index & 7))) & 1) !== 0;
                    index++;
                }
            }
        }
    }

    /** @param {number} mask */
    const apply = (mask) => {
        for (let y = 0; y < size; y++) {
            for (let x = 0; x < size; x++) {
                if (!reserved[y][x] && masked(mask, x, y)) {
                    modules[y][x] = !modules[y][x];
                }
            }
        }
    };

    let best = 0;
    let lowest = Infinity;
    for (let mask = 0; mask < 8; mask++) {
        apply(mask);
        format(mask);

        const score = penalty(modules);
        if (score < lowest) {
            lowest = score;
            best = mask;
        }

        apply(mask);
    }

    apply(best);
    format(best);

    return modules;
}

/**
 * @param {HTMLCanvasElement} canvas
 * @param {string} text
 * @param {number} scale pixels per module
 */
export function qrDraw(canvas, text, scale = 6) {
    const modules = qrEncode(text);
    const quiet = 4;
    const side = (modules.length + (2 * quiet)) * scale;

    canvas.width = side;
    canvas.height = side;

    const ctx = canvas.getContext("2d");
    if (!ctx) {
        return;
    }

    ctx.fillStyle = "#fff";
    ctx.fillRect(0, 0, side, side);
    ctx.fillStyle = "#000";

    modules.forEach((row, y) => {
        row.forEach((dark, x) => {
            if (dark) {
                ctx.fillRect((x + quiet) * scale, (y + quiet) * scale, scale, scale);
            }
        });
    });
}
