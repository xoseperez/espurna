import globals from "globals";
import js from "@eslint/js";

export default [
    {
        ignores: [
            "libraries/**/*",
            "espurna/**/*",
            "hardware/**/*",
            "scripts/**/*",
            "test/**/*",
            "**/*.js",
        ],
    },
    {
        ...js.configs.recommended,
        languageOptions: {
            "globals": {
                ...globals.es2022,
            },
        },
        rules: {
            "no-unused-vars": ["error", {
                "argsIgnorePattern": "^_",
                "caughtErrorsIgnorePattern": "^_",
            }],
        }
    },
    {
        files: [
            "gulpfile.mjs",
            "vite.config.mjs",
            "html/*.mjs",
            "html/preset/**/*.mjs",
        ],
        languageOptions: {
            "globals": {
                ...globals.node,
            }
        },
        rules: {
            "quotes": ["error", "single"],
            "no-throw-literal": "error",
        }
    },
    {
        files: [
            "html/spec/**/*.mjs",
            "html/src/**/*.mjs",
        ],
        languageOptions: {
            "globals": {
                ...globals.browser,
            }
        },
        rules: {
            "no-invalid-this": "error",
            "eqeqeq": "error",
            "prefer-arrow-callback": "error"
        }
    }
];
