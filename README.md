[Imgur](https://i.imgur.com/eN1nTjc.png)[Imgur](https://i.imgur.com/eN1nTjc.png)
# OIL: Obvious Imperative Language
## Description

OIL is an readable, imperative, statically typed C-like language,
designed to be readable, and obvious in its syntax.

## Ideal Syntax (subject to change)
```rust
import std.io;
import std.memory;
import std.math;

type String = [char];

type Stack(type T) impl Index
{
    list: [T];
    size: uint;

    fn new(): Stack {
        this: Stack;

        list: [T] = memory.alloc(T, 256);

        this.list = list;
        this.size = 0;

        return this;
    };

    impl fn index(this, pos: uint) {
        return this.list[pos];
    };

    fn length(this): uint { return this.size; };

    fn capacity(this): uint { return this.list.length(); };

    fn push(this, item: T) {
        if (this.size == this.list.length()) {
            this.list.realloc(this.list.length() + math.max(this.size / 2, 1));
        }
        this.list[this.size] = item;
        this.size += 1;
    };

    fn free(this) {
        this.list.free();
    };
};

fn main() {
    stack: Stack = Stack.new();

    i: uint = 0;
    while i < stack.length() {
        io.printf("list[%] = %\n", i, list[i]);
        i += 1;
    };

    // we will have some form of RAII or garbage collection in future
    list.free();
};
```

## End Goal

Our end goal for this project is to have basic .WAV file synthesis.
