var Stat = require('fs').Stats

module.exports = cloneStats

function cloneStats(stats) {
  var replacement = Object.create(Object.getPrototypeOf(stats))

  Object.keys(stats).forEach(function(key) {
    replacement[key] = stats[key]
  })

  return replacement
}
