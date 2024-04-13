"use strict";


WebAssembly.instantiateStreaming(fetch("./compiler.wasm"))
    .then(async ({ instance }) => {
        console.log(instance);
    });