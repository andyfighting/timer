# Timer
An userspace application timer based on the minimum heap algorithm.

# How to use:
```cpp
init_sys_timer();
create_sys_timer(callback, 1, type, (void *)args);
free_sys_timer();
```
type is usually set at `cycle_timer`.  
args: callback function `parameter`.

```cpp
timer_type type = cycle_timer
```
