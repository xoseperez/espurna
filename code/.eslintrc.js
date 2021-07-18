module.exports = {
    "env": {
        "es2021": true,
        "browser": true
    },
    "parserOptions": {
        "ecmaVersion": 2021
    },
    "extends": [
        "eslint:recommended"
    ],
    "globals": {
        "document": "readonly",
        "navigator": "readonly",
        "window": "readonly",
        "iro": "readonly"
    },
    "rules": {
        "no-invalid-this": "error",
        "eqeqeq": "error",
        "prefer-arrow-callback": "error"
    }
};
