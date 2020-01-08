# qvariant-value

Replace uses of QVariant::value with qvariant_cast

## Known bugs

When const is used before the type, it is not properly repeated in the qvariant_cast.
`foo.value<const Foo*>()`  is replaced with `qvariant_cast<Foo*>(foo)`.

