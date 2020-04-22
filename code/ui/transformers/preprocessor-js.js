const createHash = require("crypto").createHash;
const babelJest = require("babel-jest");
const prepro = require("webpack-preprocessor-loader");
const query = require("./query");

module.exports = {
    getCacheKey(fileData, filename, ...rest) {
        const babelCacheKey = babelJest.getCacheKey(fileData, filename, ...rest);

        return createHash("md5")
            .update(babelCacheKey)
            .update(JSON.stringify(query))
            .digest("hex");
    },
    process(src, filename, ...rest) {
        const transformedContent = prepro.call({query}, src);

        return babelJest.process(transformedContent, filename, ...rest);
    },
};
