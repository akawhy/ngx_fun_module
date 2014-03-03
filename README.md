## ngx_fun_module

A hello world module

## conf

```
location / {
    set $funny  "good morning";
    fun         funny;
    fun_value   "good night";

    content_by_lua '
        local v = ngx.var.funny
        if v then
            ngx.say(v)
        end
    ';
}
```
