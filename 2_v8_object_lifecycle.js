const addon = require('./2_v8_object_lifecycle.node')

function callback(obj, pobj) {
    console.log('callback with obj', obj, pobj)
}

function foo() {
    const local = {};
    addon.setCallback(callback);
    addon.setAsLocal(local, true);
    addon.callCallback()
}

foo()