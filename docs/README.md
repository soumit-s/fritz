# Methods
Methods or functions can be declared using either the `method` keyword or using the `->` operator.

1. Using the `method` keyword
   ```
   method <name> (<parameter-list>) {
      # Body
   }
   ```
   Example of a function that adds two numbers
   ```
   method add(a, b) { return a + b }
   ```

2. Using the `->` operator
   ```
   (<parameter-list>) -> { ... }
   ```
   You cannot directly specify the name of the method when defined in this way. However, it must be noted
   that methods donot have a fixed name. In fact, they donot have a name at all. Instead, they take the
   name of the variable in which they are stored.

   The `add` method using this syntax.
   ```
   add = (a, b) -> { return a + b }
   # Note how we assign the closer to a variable.
   # Now it can be called as, 
   add(24, 25) # = 49
   ```

   It must be noted that methods declared using the `->` operator are referred to as closures.

# Pattern matching
The `=` operator is the pattern matching operator. The simplest example of pattern matching is
```
x = y
```
Here, `x` is the pattern and `y` is the payload.
Another example would be unpacking certain properties of an object and storing their
values in a variable having the same name.
```
x = { a=24 b=54 c=72 d='hello' e='<3'}

# Creates 3 variables named a, b, and c and sets their 
# values to x.a, x.b, and x.c respectively.
{a, b, c} = x

# Another way to do the above would be 
# scope.{a, b, c} = x

```

In a pattern, the `.{ ... }` signifies object unpacking. Object unpacking can 
be used to modify(or add) certain properties of an object. For example,
```
``` 


# [] Operator

The [...] Operator has differing behaviour.

1. When used in the form [a, b, c], it creates a List with the elements a, b, c i.e the above is equivalent  
   to `new List(a, b, c)`

2. When used in the form x[y], it acts as an array access operator. Internally, it calls the __slicer method with   
   the argument `y`. 
   `x[y] -> x.__slicer(y)`
   Note, here `y` can be any expression.
  
  

# Using native methods from external library
In order to load and use symbols defined in an 
external library, the universal function loadLib()
has to be used. It takes a path to the dynamic library
and returns a DyLibrary object.

# Embed (Complex) Strings

Embed Strings also known as Complex Strings can be used to declare strings with values directly embedded into them
It is similar to `f"..."` in Python or `` `...` `` in JavaScript.  


In order to define a complex string use double quotes ( `"` ) and to embed values use `{{ ... }}`. For example
```
name = 'Fritz'
age = 24
x = "{{name}} just turned {{age}}"

out(x) # Fritz just turned 24
```

Any valid expression can go within `{{ }}`, however it must be one expression only.  


Moreover the value that is to be displayed must have a __toStr() method in it.  
For primitive types such as int, float, boolean this method has been 
defined inside their respective Wrappers. If this method is not defined in an
object then the raw representation is used to embed into the string.

# Getters and Setters
Getters and setters are used to access and change the value of a property respectively.
Unlike most other languages where the getter and setter are nothing but a syntactic sugar like
`x.a = b` which ultimately gets translated to `x.a(b)` where the defintion of `a` is `get a() { return _a; }` 
where `_a` is the shadow property that is used to store the value, and is kept hidden from the user.  
  
However, in Fritz the getter and setter are tightly bound to the property, i.e there is no need for a shadow property 
to hold the value of the property.  
  
In order to define a getter and setter, use the attach (`:`) operator.
```
x.a:{
   # Getter
   method get(o) { ... }

   # Setter
   method set(o, v) { ... }
}
```

1. Getter: The getter must take one argument, which is the value of the property.

2. Setter: The setter must take two arguments(o, v), the first being the value of the property and the 
   second argument being the value to which the property's value must be changed. The value returned by the setter
   method will become the new value of the property.


An example of getters and setters that mimic the default behaviour i.e without getters and setters.
```
x.a: {
   get = (o) -> o
   set = (o, v) -> v
}
```