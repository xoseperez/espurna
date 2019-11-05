/*
 * Copyright (c) Adrien Foulon - 2019.
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


module.exports = {
    parserOptions: {
        parser: "babel-eslint",
    },
    extends: [
        // add more generic rulesets here, such as:
        // 'eslint:recommended',
        'plugin:vue/recommended'
    ],
    rules: {
        "vue/html-indent": ["error", 4, {
            "attribute": 1,
            "baseIndent": 1,
            "closeBracket": 0,
            "alignAttributesVertically": true,
            "ignores": []
        }],
        "vue/name-property-casing": ["error", "kebab-case"],
        "vue/mustache-interpolation-spacing": ["error", "never"],
        "vue/max-attributes-per-line": ["error", {
            "singleline": 5,
            "multiline": {
                "max": 3,
                "allowFirstLine": true
            }
        }],
        "vue/html-closing-bracket-newline": ["error", {
            "singleline": "never",
            "multiline": "never"
        }],
        "vue/html-closing-bracket-spacing": ["error", {
            "startTag": "never",
            "endTag": "never",
            "selfClosingTag": "never"
        }],
        "vue/singleline-html-element-content-newline": ["warning", {
            "ignoreWhenNoAttributes": true,
            "ignoreWhenEmpty": true,
            "ignores": ["pre", "textarea", "v-btn", 'v-icon']
        }],
        "vue/html-self-closing": ["error", {
            "html": {
                "void": "never",
                "normal": "never",
                "component": "always"
            },
            "svg": "always",
            "math": "always"
        }],
    }
};