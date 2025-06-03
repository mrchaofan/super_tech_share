const { Worker } = require('worker_threads');

const worker = new Worker(require.resolve('./4_node_worker_worker_thread'));

worker.on('message', (msg) => {
    if (msg.event === 'result') {
        console.log('收到计算结果', msg)
    }
})


