### configure
Shows and modifies global configurations.

Usage: `configure [key [value]]`
<pre>
If no keys are provided, it will list all configuration keys and values.
If a key is provided, but no value given, it will only show the value of such key.
If a key and value are provided, it will set the value of that key.

Possible keys:
 - max_nodes_in_cache      Max nodes loaded in memory.
                           This controls the number of nodes that the SDK stores in memory.
 - exported_folders_sdks   Number of additional SDK instances loaded at startup.
                           This controls the number of SDK instances that are created at
                           startup in order to download or import contents from exported
                           folder links. Default 5. Min 0. Max 20. If set to 0, you will not
                           be able to download or import from folder links.
</pre>
