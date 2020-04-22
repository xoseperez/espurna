const vueJest = require("vue-jest");
const prepro = require("webpack-preprocessor-loader");
const query = require("./query");

module.exports = {
    process(src, filename, ...rest) {
        const transformedContent = prepro.call({query}, src);

        return vueJest.process(transformedContent, filename, ...rest);
    },
};
