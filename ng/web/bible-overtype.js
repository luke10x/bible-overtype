const importObject = {
  imports: {
    imported_func(arg) {
      console.log(arg);
    },
  },
};

fetch("./bible-overtype.wasm")
  .then((response) => response.arrayBuffer())
  .then((bytes) => WebAssembly.instantiate(bytes, importObject))
  .then((module) => module.instance.exports.bereshit())
  .then(result => {
    var term = new Terminal();
    term.open(document.getElementById('terminal'));
    term.write('Hedllo from \x1B[1;3;31mxterm.js\x1B[0m $ ')
    term.write(` R=${result};`);
    console.log({result});

  })

