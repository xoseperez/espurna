import globals from 'globals';
import js from '@eslint/js';

import {
    BUILD_SCRIPTS,
    TEST_SCRIPTS,
    SOURCE_SCRIPTS,
} from './gulpfile.mjs';

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
    {
        ...js.configs.recommended,
        languageOptions: {
            'globals': {
                ...globals.es2022,
            },
        },
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
            'quotes': ['error', 'single'],
            'no-throw-literal': 'error',
        }
    },
    {
        files: [
            ...SOURCE_SCRIPTS,
            ...TEST_SCRIPTS,
        ],
        languageOptions: {
            'globals': {
                ...globals.browser,
            }
        },
        rules: {
            'no-invalid-this': 'error',
            'eqeqeq': 'error',
            'prefer-arrow-callback': 'error'
        }
    }
];
