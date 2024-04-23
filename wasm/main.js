"use strict";

/** @type WebAssembly.Instance */
let instance;

/** @type HTMLCanvasElement */
const canvas = document.getElementById("canvas");
const context = canvas.getContext("2d", { alpha: false });

const onDraw = (timeElapsed) => {
    const width = context.canvas.width;
    const height = context.canvas.height;
    const imageDataPtr = instance.exports.draw(width, height);

    const imageDataArray = new Uint8ClampedArray(instance.exports.memory.buffer, imageDataPtr, width * height * 4);
    const imageData = new ImageData(imageDataArray, width, height);

    context.putImageData(imageData, 0, 0);

    requestAnimationFrame(onDraw);
}

const setCanvasWidth = () => {
    context.canvas.width = window.innerWidth;
    context.canvas.height = window.innerHeight;
}
setCanvasWidth();
window.onresize = setCanvasWidth;

WebAssembly.instantiateStreaming(fetch("./main.wasm"))
    .then((module) => {
        instance = module.instance;
        window.instance = instance;
        instance.exports.init();
        requestAnimationFrame(onDraw)
    });