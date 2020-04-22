module.exports = {
  preset: "@vue/cli-plugin-unit-jest",
  transform: {
    "^.+\\.js$": "<rootDir>/transformers/preprocessor-js.js",
    "^.+\\.vue$": "<rootDir>/transformers/preprocessor-vue.js",
    "^.+\\.svg$": "<rootDir>/transformers/svg.js"
  },
};
