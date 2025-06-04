const addon = require('./build/Release/addon.node')

console.log(addon.hello());

addon.crash()

console.log(addon.hello());
