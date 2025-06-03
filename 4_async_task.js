const { MyObject } = require('./3_async_task.node')
const { EventEmitter } = require('events')

MyObject.prototype._init = function _init() {
    EventEmitter.call(this)
}

MyObject.prototype.__proto__ = EventEmitter.prototype

const myObject = new MyObject()

myObject.on('result', (result) => {
    console.log('长任务执行耗时', Date.now() - time + 'ms', result)
})

let time = Date.now()

console.log('开始执行阻塞长任务');

let result = myObject.longTaskSync()

console.log('长任务执行耗时', Date.now() - time + 'ms', result)

time = Date.now()
console.log('开始执行非阻塞长任务');
myObject.longTaskAsync();

console.log('任务开始后下一行代码');