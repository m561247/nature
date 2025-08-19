# [builtin](https://github.com/nature-lang/nature/blob/master/std/builtin/builtin.n)

The builtin contains global functions and types that don't need to be imported.

## type nullable

```
type nullable<T> = T?
```

Nullable type that can hold a value of type T or null.

## fn print

```
fn print(...[any] args)
```

Print arguments to standard output without newline.

## fn println

```
fn println(...[any] args)
```

Print arguments to standard output with newline.

## fn panic

```
fn panic(string msg)
```

Panic with the given message and terminate the program.

## fn assert

```
fn assert(bool cond)
```

Assert that the condition is true, panic if false.

# [chan](https://github.com/nature-lang/nature/blob/master/std/builtin/chan.n)

## fn chan_new

```
fn chan_new<T>(...[int] args):chan<T>
```

Create a new channel with optional buffer size.

## type chan

### chan.send

```
fn chan<T>.send(T msg):void!
```

Send a message to the channel, blocks if channel is full.

### chan.try_send

```
fn chan<T>.try_send(T msg):bool!
```

Try to send a message to the channel without blocking.

### chan.on_send

```
fn chan<T>.on_send(T msg):void
```

Event handler for channel send operations.

### chan.recv

```
fn chan<T>.recv():T!
```

Receive a message from the channel, blocks if channel is empty.

### chan.on_recv

```
fn chan<T>.on_recv():T
```

Event handler for channel receive operations.

### chan.try_recv

```
fn chan<T>.try_recv():(T, bool)!
```

Try to receive a message from the channel without blocking.

### chan.close

```
fn chan<T>.close():void!
```

Close the channel.

### chan.is_closed

```
fn chan<T>.is_closed():bool
```

Check if the channel is closed.

### chan.is_successful

```
fn chan<T>.is_successful():bool
```

Check if the last operation was successful.

# [coroutine](https://github.com/nature-lang/nature/blob/master/std/builtin/coroutine.n)

## type future_t

```
type future_t<T> = struct{
    i64 size
    rawptr<T> result
    throwable? error
    anyptr co
}
```

Future type for asynchronous operations.

### future_t.await

```
fn future_t<T>.await():T!
```

Wait for the future to complete and return the result.

### future_t.await (void)

```
fn future_t<T:void>.await():void!
```

Wait for the future to complete (void return type).

## fn async

```
fn async<T>(fn():void! function, int flag):ptr<future_t<T>>
```

Execute a function asynchronously and return a future.

## fn co_return

```
fn co_return<T>(rawptr<T> result)
```

Return a result from a coroutine.

# [error](https://github.com/nature-lang/nature/blob/master/std/builtin/error.n)

## type throwable

```
type throwable = interface{
    fn msg():string
}
```

Interface for throwable objects.

## type errort

```
type errort:throwable = struct{
    string message
    bool is_panic
}
```

Error type implementing throwable interface.

### errort.msg

```
fn errort.msg():string
```

Get the error message.

## type errable

```
type errable<T> = errort|T
```

Union type for error handling.

## fn errorf

```
fn errorf(string format, ...[any] args):ptr<errort>
```

Create a formatted error.

# [map](https://github.com/nature-lang/nature/blob/master/std/builtin/map.n)

## fn map_new

```
fn map_new<T,U>():map<T,U>
```

Create a new map with key type T and value type U.

## type map

### map.len

```
fn map<T,U>.len():int
```

Get the number of key-value pairs in the map.

### map.del

```
fn map<T,U>.del(T key)
```

Delete a key-value pair from the map.

### map.contains

```
fn map<T,U>.contains(T key):bool
```

Check if the map contains the given key.

# [set](https://github.com/nature-lang/nature/blob/master/std/builtin/set.n)

## fn set_new

```
fn set_new<T>():set<T>
```

Create a new set with element type T.

## type set

### set.add

```
fn set<T>.add(T key)
```

Add an element to the set.

### set.contains

```
fn set<T>.contains(T key):bool
```

Check if the set contains the given element.

### set.del

```
fn set<T>.del(T key)
```

Remove an element from the set.

# [string](https://github.com/nature-lang/nature/blob/master/std/builtin/string.n)

## type string

### string.len

```
fn string.len():int
```

Get the length of the string.

### string.ref

```
fn string.ref():anyptr
```

Get a pointer to the string data.

### string.char

```
fn string.char():u8
```

Get the first character of the string.

# [vec](https://github.com/nature-lang/nature/blob/master/std/builtin/vec.n)

## fn vec_new

```
fn vec_new<T>(T value, int len):vec<T>
```

Create a new vector with initial value and length.

## fn vec_cap

```
fn vec_cap<T>(int cap):vec<T>
```

Create a new vector with specified capacity.

## type vec

### vec.push

```
fn vec<T>.push(T v)
```

Add an element to the end of the vector.

### vec.append

```
fn vec<T>.append(vec<T> l2)
```

Append another vector to this vector.

### vec.slice

```
fn vec<T>.slice(int start, int end):vec<T>
```

Create a slice of the vector from start to end.

### vec.concat

```
fn vec<T>.concat(vec<T> l2):vec<T>
```

Concatenate two vectors and return a new vector.

### vec.copy

```
fn vec<T>.copy(vec<T> src):int
```

Copy elements from source vector to this vector.

### vec.len

```
fn vec<T>.len():int
```

Get the number of elements in the vector.

### vec.cap

```
fn vec<T>.cap():int
```

Get the capacity of the vector.

### vec.ref

```
fn vec<T>.ref():anyptr
```

Get a pointer to the vector data.

### vec.sort

```
fn vec<T>.sort(fn(int, int):bool less)
```

Sort the vector using the provided comparison function.

### vec.search

```
fn vec<T>.search(fn(int):bool predicate):int
```

Binary search in the vector using a predicate function.