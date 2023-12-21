query -> CREATED CURSOR
batch request -> collect min(MAX, REMAINING)
batch accept <- rows
    ...
batch terminate -> flush query
finish <- count
