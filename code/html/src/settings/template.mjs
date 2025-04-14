/**
 * @param {string} name
 * @returns {DocumentFragment}
 */
export function loadTemplate(name) {
    const template = /** @type {HTMLTemplateElement} */
        (document.getElementById(`template-${name}`));
    return document.importNode(template.content, true);
}

/**
 * @param {HTMLElement} target
 * @param {DocumentFragment} template
 * @returns {Element | null}
 */
export function mergeTemplate(target, template) {
    for (let child of Array.from(template.children)) {
        target.appendChild(child);
    }

    return target.lastElementChild;
}

