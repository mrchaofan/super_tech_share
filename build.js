#!/usr/bin/env node
const { exec } = require('child_process')

const filename = process.argv.slice(2)[0] || ''

if (!filename) {
    console.log('No input file provided to compile. Bye~')
    process.exit(1)
}

const normalName = filename.replace(/\.cc$/, '')

exec(`clang++ -g -shared -fPIC -std=c++20 -undefined dynamic_lookup -I./include ${normalName}.cc -o ${normalName}.node`, (er, stdout, stderr) => {
    console.log(stdout)
    console.log(stderr)
    console.error(er)
})