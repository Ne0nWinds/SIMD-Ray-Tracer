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

    if (document.hidden || !document.hasFocus()) {
        instance.exports.resetKeyState();
    }

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

const convertJSCodeToWasmCode = (code) => {
    let k = 0 | 0;
    switch (code) {
        case "Escape": {
            k = 1; // key::Escape
        } break;
        case "Digit1": {
            k = 2; // key::One
        } break;
        case "Digit2": {
            k = 3; // key::Two
        } break;
        case "Digit3": {
            k = 4; // key::Three
        } break;
        case "Digit4": {
            k = 5; // key::Four
        } break;
        case "Digit5": {
            k = 6; // key::Five
        } break;
        case "Digit6": {
            k = 7; // key::Six
        } break;
        case "Digit7": {
            k = 8; // key::Seven
        } break;
        case "Digit8": {
            k = 9; // key::Eight
        } break;
        case "Digit9": {
            k = 10; // key::Nine
        } break;
        case "Digit0": {
            k = 11; // key::Zero
        } break;
        case "Minus": {
            k = 12; // key::Minus
        } break;
        case "Equal": {
            k = 13; // key::Plus
        } break;
        case "Backspace": {
            k = 14; // key::Backspace
        } break;
        case "Tab": {
            k = 15; // key::Tab
        } break;
        case "KeyQ": {
            k = 16; // key::Q
        } break;
        case "KeyW": {
            k = 17; // key::W
        } break;
        case "KeyE": {
            k = 18; // key::E
        } break;
        case "KeyR": {
            k = 19; // key::R
        } break;
        case "KeyT": {
            k = 20; // key::T
        } break;
        case "KeyY": {
            k = 21; // key::Y
        } break;
        case "KeyU": {
            k = 22; // key::U
        } break;
        case "KeyI": {
            k = 23; // key::I
        } break;
        case "KeyO": {
            k = 24; // key::O
        } break;
        case "KeyP": {
            k = 25; // key::P
        } break;
        case "BracketLeft": {
            k = 26; // key::LeftBracket
        } break;
        case "BracketRight": {
            k = 27; // key::RightBracket
        } break;
        case "Enter": {
            k = 28; // key::Enter
        } break;
        case "ControlLeft": {
            k = 29; // key::LeftControl
        } break;
        case "KeyA": {
            k = 30; // key::A
        } break;
        case "KeyS": {
            k = 31; // key::S
        } break;
        case "KeyD": {
            k = 32; // key::D
        } break;
        case "KeyF": {
            k = 33; // key::F
        } break;
        case "KeyG": {
            k = 34; // key::G
        } break;
        case "KeyH": {
            k = 35; // key::H
        } break;
        case "KeyJ": {
            k = 36; // key::J
        } break;
        case "KeyK": {
            k = 37; // key::K
        } break;
        case "KeyL": {
            k = 38; // key::L
        } break;
        case "Semicolon": {
            k = 39 // key::Semicolon
        } break;
        case "Quote": {
            k = 40 // key::Quote
        } break;
        case "Backquote": {
            k = 41 // key::GraveAccent
        } break;
        case "ShiftLeft": {
            k = 42 // key::LeftShift
        } break;
        case "Backslash": {
            k = 43 // key::Pipe
        } break;
        case "Z": {
            k = 44 // key::Z
        } break;
        case "KeyX": {
            k = 45 // key::X
        } break;
        case "KeyC": {
            k = 46 // key::C
        } break;
        case "KeyV": {
            k = 47 // key::V
        } break;
        case "KeyB": {
            k = 48 // key::B
        } break;
        case "KeyN": {
            k = 49 // key::N
        } break;
        case "KeyM": {
            k = 50 // key::M
        } break;
        case "Comma": {
            k = 51 // key::Comma
        } break;
        case "Period": {
            k = 52 // key::Period
        } break;
        case "Slash": {
            k = 53 // key::QuestionMark
        } break;
        case "ShiftRight": {
            k = 54 // key::RightShift
        } break;
        case "NumpadMultiply--": {
            k = 55 // key::NumpadMultiply
        } break;
        case "AltLeft": {
            k = 56 // key::LeftAlt
        } break;
        case "Space": {
            k = 57 // key::Space
        } break;
        case "CapsLock": {
            k = 58 // key::CapsLock
        } break;
        case "F1": {
            k = 59 // key::F1
        } break;
        case "F2": {
            k = 60 // key::F2
        } break;
        case "F3": {
            k = 61 // key::F3
        } break;
        case "F4": {
            k = 62 // key::F4
        } break;
        case "F5": {
            k = 63 // key::F5
        } break;
        case "F6": {
            k = 64 // key::F6
        } break;
        case "F7": {
            k = 65 // key::F7
        } break;
        case "F8": {
            k = 66 // key::F8
        } break;
        case "F9": {
            k = 67 // key::F9
        } break;
        case "F10": {
            k = 68 // key::F10
        } break;
        case "Pause--": {
            k = 69 // key::Pause
        } break;
        case "ScrollLock": {
            k = 70 // key::ScrollLock
        } break;
        case "Numpad7": {
            k = 71 // key::Numpad7
        } break;
        case "Numpad8": {
            k = 72 // key::Numpad8
        } break;
        case "Numpad9": {
            k = 73 // key::Numpad9
        } break;
        case "NumpadMinus": {
            k = 74 // key::NumpadMinus
        } break;
        case "Numpad4": {
            k = 75 // key::Numpad4
        } break;
        case "Numpad5": {
            k = 76 // key::Numpad5
        } break;
        case "Numpad6": {
            k = 77 // key::Numpad6
        } break;
        case "NumpadPlus": {
            k = 78 // key::NumpadPlus
        } break;
        case "Numpad1": {
            k = 79 // key::Numpad1
        } break;
        case "Numpad2": {
            k = 80 // key::Numpad2
        } break;
        case "Numpad3": {
            k = 81 // key::Numpad3
        } break;
        case "Numpad0": {
            k = 82 // key::Numpad0
        } break;
        case "NumpadPeriod": {
            k = 83 // key::NumpadPeriod
        } break;
        case "AltPrintScreen": {
            k = 84 // key::AltPrintScreen
        } break;
        case "_Unused": {
            k = 85 // key::_Unused
        } break;
        case "OEM10": {
            k = 86 // key::OEM10
        } break;
        case "F11": {
            k = 87 // key::F11
        } break;
        case "F12": {
            k = 88 // key::F12
        } break;
        case "MetaLeft": {
            k = 89 // key::LeftWindows
        } break;
        case "AltRight": {
            k = 90 // key::RightAlt
        } break;
        case "MetaRight": {
            k = 91 // key::RightWindows
        } break;
        case "Menu": {
            k = 92 // key::Menu
        } break;
        case "ControlRight": {
            k = 93 // key::RightControl
        } break;
        case "Insert": {
            k = 94 // key::Insert
        } break;
        case "Home": {
            k = 95 // key::Home
        } break;
        case "PageUp": {
            k = 96 // key::PageUp
        } break;
        case "Delete": {
            k = 97 // key::Delete
        } break;
        case "End": {
            k = 98 // key::End
        } break;
        case "PageDown": {
            k = 99 // key::PageDown
        } break;
        case "ArrowUp": {
            k = 100 // key::ArrowUp
        } break;
        case "ArrowLeft": {
            k = 101 // key::ArrowLeft
        } break;
        case "ArrowDown": {
            k = 102 // key::ArrowDown
        } break;
        case "ArrowRight": {
            k = 103 // key::ArrowRight
        } break;
        case "NumLock": {
            k = 104 // key::NumLock
        } break;
        case "NumpadForwardSlash": {
            k = 105 // key::NumpadForwardSlash
        } break;
        case "NumpadEnter": {
            k = 106 // key::NumpadEnter
        } break;
    }
    return k;
}

window.onkeydown = (e) => {
    if (document.hasFocus()) {
        e.preventDefault();
        let k = convertJSCodeToWasmCode(e.code);
        instance.exports.updateKeyState(k, 1);
    }
}
window.onkeyup = (e) => {
    if (document.hasFocus()) {
        e.preventDefault();
        let k = convertJSCodeToWasmCode(e.code);
        instance.exports.updateKeyState(k, 0);
    }
}