module.exports = {
    "presets": [
        [
            "@babel/env",
            {
                "targets": {
                    "chrome": "77"
                }
            }
        ]
    ],
    "plugins": [
        [
            "component",
            {
                "libraryName": "element-ui",
                "style": false
            }
        ]
    ]
};
