const { MyObject } = require('./2_v8_object_template.node')
const { EventEmitter } = require('events')

MyObject.prototype._init = function _init() {
    EventEmitter.call(this)
}

MyObject.prototype.__proto__ = EventEmitter.prototype

const myObject = new MyObject()

myObject.on('valuechanged', () => {
    console.log('valuechanged', myObject.value);
});

myObject.plusOne();
