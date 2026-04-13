import * as path from 'node:path';

import globals from 'globals';
import js from '@eslint/js';

import stylistic from '@stylistic/eslint-plugin'

import {
    ROOT,
    BUILD_SCRIPTS as RAW_BUILD_SCRIPTS,
    TEST_SCRIPTS as RAW_TEST_SCRIPTS,
    SOURCE_SCRIPTS as RAW_SOURCE_SCRIPTS,
} from './gulpfile.mjs';

/** @param {string} to */
function relativeToRoot(to) {
    return path.relative(ROOT, to);
}

const BUILD_SCRIPTS = RAW_BUILD_SCRIPTS.map(relativeToRoot);
const TEST_SCRIPTS = RAW_TEST_SCRIPTS.map(relativeToRoot);
const SOURCE_SCRIPTS = RAW_SOURCE_SCRIPTS.map(relativeToRoot);

export default [
    {
        ignores: [
            'libraries/**/*',
            'espurna/**/*',
            'hardware/**/*',
            'scripts/**/*',
            'test/**/*',
            '**/*.js',
        ],
    },
    js.configs.recommended,
    {
        plugins: {
            '@stylistic': stylistic,
        },
    },
    {
        languageOptions: {
            'globals': {
                ...globals.es2022,
            },
        },
    },
    {
        rules: {
            'no-unused-vars': ['error', {
                'argsIgnorePattern': '^_',
                'caughtErrorsIgnorePattern': '^_',
            }],
        }
    },
    {
        files: BUILD_SCRIPTS,
        languageOptions: {
            'globals': {
                ...globals.node,
            }
        },
        rules: {
            '@stylistic/quotes': ['error', 'single'],
            'no-throw-literal': 'error',
            'eqeqeq': 'error',
            'require-yield': 'off',
        }
    },
    {
        files: [
            ...SOURCE_SCRIPTS,
            ...TEST_SCRIPTS,
        ],
        languageOptions: {
            'globals': {
                ...globals.node,
                ...globals.browser,
            }
        },
        rules: {
            'eqeqeq': 'error',
            'no-invalid-this': 'error',
            'prefer-arrow-callback': 'error',
            'no-throw-literal': 'error',
        }
    }
];
