

* Create one set_value function per type, which can also double as a add_text function
* Create "void* get_<element>_<attribute>" function for each attribute and combine this with the set_value function in the attribute info table
* Move everything except the element info tables from the schema namespace to the anonymous namespace 
* Replace unordered_map in ElementContext with an array (and limit child element types to something like 128)