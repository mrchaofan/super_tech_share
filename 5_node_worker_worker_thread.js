const { parentPort } = require('worker_threads');
const { MyObject } = require('./3_async_task.node')
const { EventEmitter } = require('events')

MyObject.prototype._init = function _init() {
    EventEmitter.call(this)
}

MyObject.prototype.__proto__ = EventEmitter.prototype

const myObject = new MyObject()

let result = myObject.longTaskSync()

parentPort.postMessage({
    event: 'result',
    data: result
})