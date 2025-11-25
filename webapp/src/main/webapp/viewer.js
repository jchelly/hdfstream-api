"use strict";

import { marked } from 'https://unpkg.com/marked@16.0.0/lib/marked.esm.js';

function set_inner_html(node, text) {
    node.innerHTML = DOMPurify.sanitize(text);
}

// Collapse any repeated slashes in a path into one, and remove leading or
// trailing slashes
function sanitize_path(path) {

    if(path==null)return path;
    let path1 = path;
    let path2 = path;
    do {
        path1 = path2;
        path2 = path1.replace("//","/");
    } while (path1 != path2);

    if(path1.startsWith("/"))
        path1 = path1.slice(1);

    if(path1.endsWith("/"))
        path1 = path1.slice(0, -1);

    return path1;
}

// Decode a URL encoded path
function decode_path(path) {

    const components = path.split("/");
    let result = "";
    for(let i=0; i<components.length; i+=1) {
        const component = decodeURIComponent(components[i]);
        if(i == 0)
            result = component;
        else
            result = result + "/" + component;
    }
    return result;
}

function add_element(node, type) {
    return node.appendChild(document.createElement(type));
}

function add_text(node, text) {
    return node.appendChild(document.createTextNode(text));
}

function format_file_size(size) {
    var i = (size == 0) ? 0 : Math.floor(Math.log(size) / Math.log(1024));
    return +((size / Math.pow(1024, i)).toFixed(2)) * 1 + ' ' + ['B', 'kB', 'MB', 'GB', 'TB', 'PB', 'EB'][i];
}

function format_shape(shape) {
    let result = "[";
    for(let i=0; i<shape.length; i++) {
        result += shape[i].toString();
        if(i<shape.length-1)result+=", ";
    }
    result += "]";
    return result;
}

function join_path(path, name) {
    if(path == "/")
        return "/" + name;
    else
        return path+"/"+name;
}

function msgpack_url(path, object=null, data_size_limit=null) {
    const url = new URL("/hdfstream/msgpack/"+path, window.location.origin);
    if(object != null)
        url.searchParams.set("object", object);
    if(data_size_limit != null)
        url.searchParams.set("data_size_limit", data_size_limit);
    return url;
}

function download_url(path) {
    const url = new URL("/hdfstream/download/"+path, window.location.origin);
    return url;
}

function viewer_url(path, object=null, page=null) {
    let url = null;
    if(page == null) {
        // Generate URL associated with a path and object
        url = new URL("/hdfstream/viewer/"+path, window.location.origin);
        if(object != null)
            url.searchParams.set("object", encodeURIComponent(object));
    } else {
        // Generate URL of a static page
        url = new URL("/hdfstream/viewer", window.location.origin);
            url.searchParams.set("page", encodeURIComponent(page));
    }
    return url;
}

function docs_url(page) {
    const url = new URL("/hdfstream/static/"+page+".html", window.location.origin);
    return url;
}

function slice_url(path, object, start, count) {

    const url = new URL("/hdfstream/msgpack/"+path, window.location.origin);
    let slice_string = "";
    for(var i=0; i<start.length; i++) {
        const end = start[i] + count[i];
        slice_string += start[i].toString() + ":" + end.toString();
        if(i < start.length-1)slice_string += ",";
    }
    url.searchParams.set("object", object);
    url.searchParams.set("slice", slice_string);
    return url;
}

// Given an encoded ndarray object, extract the i'th element from the
// flattened array as a string. To do this we need to identify the
// required range of bytes and interpret them as the appropriate type.
function format_ndarray_element(object, i) {

    // Check for compound types, which we can't handle yet
    if(typeof object.type != "string")return "Unsupported type";

    // Responses >=4GB will be split over multiple msgpack_bins, which we do
    // not attempt to handle here.
    if(object.data.length > 1)return "Too large to display";

    // Get the buffer for the array
    var buffer = object.data[0].buffer.slice(object.data[0].byteOffset);

    // Check if this is a fixed length string
    if(object.type.startsWith("S")) {
        // Find the length of each element
        const length = parseInt(object.type.slice(1), 10);
        // Find offset to this element
        const offset = length*i;
        // Extract the required characters, assuming UTF-8 encoding
        const chars = buffer.slice(offset, offset+length);
        const decoder = new TextDecoder();
        const str = decoder.decode(chars);
        // API returns null padded strings, so remove any trailing null characters
        return str.replace(/\0+$/, '');
    }

    // Check for and remove endian marker
    var little_endian;
    var dtype = object.type;
    if(object.type.startsWith(">")) {
        little_endian = false;
        dtype = dtype.slice(1);
    } else {
        little_endian = true;
        dtype = dtype.slice(1);
    }
    const view = new DataView(buffer);

    // Handle signed integers
    if(dtype.startsWith("i")) {
        const length = parseInt(dtype.slice(1), 10);
        switch(length) {
        case 1:
            return view.getInt8(i*length, little_endian).toString();
        case 2:
            return view.getInt16(i*length, little_endian).toString();
        case 4:
            return view.getInt32(i*length, little_endian).toString();
        case 8:
            return view.getBigInt64(i*length, little_endian).toString();
        default:
            return "unsupported int size";
        }
    }

    // Handle unsigned integers
    if(dtype.startsWith("u")) {
        const length = parseInt(dtype.slice(1), 10);
        switch(length) {
        case 1:
            return view.getUint8(i*length, little_endian).toString();
        case 2:
            return view.getUint16(i*length, little_endian).toString();
        case 4:
            return view.getUint32(i*length, little_endian).toString();
        case 8:
            return view.getBigUint64(i*length, little_endian).toString();
        default:
            return "unsupported int size";
        }
    }

    // Handle floats
    if(dtype.startsWith("f")) {
        const length = parseInt(dtype.slice(1), 10);
        switch(length) {
        case 2:
            return view.getFloat16(i*length, little_endian).toString();
        case 4:
            return view.getFloat32(i*length, little_endian).toString();
        case 8:
            return view.getFloat64(i*length, little_endian).toString();
        default:
            return "unsupported float size";
        }
    }
    return "unknown type";
}

function format_attribute_value(object) {

    // Count total number of elements
    let nr_elements = 1;
    for(let i=0; i<object.shape.length; i+=1)
        nr_elements *= object.shape[i];

    // Don't try to display large arrays
    if(nr_elements > 20)return "[Too many elements]";
    if(object.shape.length > 1)return "[Too many dimensions]";

    let result = "";

    // Show arrays in square brackets
    if(object.shape.length > 0)
        result += "[";

    // Loop over elements
    for(let i=0; i<nr_elements; i+=1) {

        // Extract the value of this element
        if(object.nd == true) {
            // Here we need to extract an ndarray element from a binary buffer
            result += format_ndarray_element(object, i);
        } else if(object.vlen == true) {
            if(typeof object.data[i] == "string") {
                // object.data is an array of strings
                result += object.data[i];
            } else {
                // object.data is an array of encoded arrays
                result += format_attribute_value(object.data[i]);
            }
        } else {
            result += "[unknown type]";
        }

        // Add comma separator if needed
        if(i<nr_elements-1)result += ", ";
    }

    // Add closing bracket
    if(object.shape.length > 0)
        result += "]";
    return result;
}

// Replace the contents of the main display with the supplied element tree
function display_content(node) {

    const oldContent = document.getElementById("content");
    const newContent = document.createElement("div");
    newContent.className = oldContent.className;
    newContent.id = oldContent.id;
    newContent.appendChild(node);
    oldContent.replaceWith(newContent);
}

// Request a msgpack encoded file, directory or HDF5 object from the server
async function fetch_and_decode(path, object=null, data_size_limit=0) {

    const url = msgpack_url(path, object, data_size_limit);
    const response = await fetch(url, {responseType: "arraybuffer"});
    const content_type = response.headers.get('Content-Type');
    if(content_type == "application/x-msgpack") {
        const buffer = await response.arrayBuffer();
        const decoded = MessagePack.decode(buffer);
        if(response.status != 200) {
            throw new Error(decoded.error);
        }
        return decoded;
    } else {
        throw new Error("Request failed, status "+response.status);
    }
}

// Request a msgpack encoded dataset slice from the server
async function fetch_and_decode_slice(path, object, start, count) {

    const url = slice_url(path, object, start, count);
    const response = await fetch(url, {responseType: "arraybuffer"});
    const content_type = response.headers.get('Content-Type');
    if(content_type == "application/x-msgpack") {
        const buffer = await response.arrayBuffer();
        const decoded = MessagePack.decode(buffer);
        if(response.status != 200) {
            throw new Error(decoded.error);
        }
        return decoded;
    } else {
        throw new Error("Request failed, status "+response.status);
    }
}

function add_filename_header(node, path, size, link_self) {

    const header = add_element(node, "h2");
    add_text(header, "File: ");
    make_links_from_path(header, path, link_self);

    // Get file size as a string
    const file_size = format_file_size(size);

    // Add a link to download the file
    const p2 = add_element(node, "p");
    const dl_link = add_element(p2, "a");
    dl_link.href = download_url(path);
    add_text(dl_link, "Full file download ("+file_size+")");

    return;
}


function make_links_from_path(node, path, link_self) {
    // Make links to parent directories from the supplied path
    var part_path = "";
    const components = path.split("/");
    for (var i = 0; i < components.length; i++) {

        // Compute the full path and URL of this component
        if(i > 0) part_path += "/";
        part_path += components[i];
        const url = viewer_url(part_path);

        if((i < components.length-1) || link_self) {
            // Add a link to the parent node
            const this_path = part_path;
            const link = add_element(node, "a");
            link.href = url;
            link.onclick = function() {display_path(this_path, null, true) ; return false}; // returning false prevents loading the href
            add_text(link, components[i]);
        } else {
            // Final component is not a link, because it would point at the current page
            add_text(node, components[i]);
        }
        if(i < components.length-1) {
            // Add separator between components
            add_text(node, " / ");
        }
    }
}


async function display_directory(path, object) {

    const frag = document.createDocumentFragment();

    // Create header with the directory name
    let div = add_element(frag, "div");
    div.classList.add("directory_listing");

    const header = add_element(div, "h2");
    add_text(header, "Directory: ");
    make_links_from_path(header, path, false);

    // Get directory size as a string
    const dir_size = format_file_size(object.size);

    // Add a link to download the directory
    const p2 = add_element(div, "p");
    const dl_link = add_element(p2, "a");
    dl_link.href = download_url(path);
    add_text(dl_link, "Full directory download ("+dir_size+")");

    // Include note about large downloads if the directory is >100GB
    if(object.size >= 107374182400) {
        const note = add_element(div, "p");
        set_inner_html(note, "Note that you can use the <a href='/hdfstream/viewer?page=python_module'>python module</a> to extract HDF5 datasets of interest without downloading complete files, or if you have a user account on COSMA then <a href='https://cosma.readthedocs.io/en/latest/data.html#globus-online'>Globus Online or bbcp</a> might allow faster downloads.");
    }

    // Make a div to contain any description for this diretcory
    const description_div = add_element(div, "div");
    const description_url = download_url(path+"/description.md");
    const labels_url      = download_url(path+"/labels.msgpack");

    // Check for any special files
    const nr_files = Object.keys(object.files).length;
    var nr_displayed = 0;
    var labels = null;
    for(let i=0; i<nr_files; i++) {
        const name = Object.keys(object.files)[i];
        if(name == "description.md") {
            // If description.md exists we render it as markdown
            const response = await fetch(description_url);
            if(response.status == 200) {
                set_inner_html(description_div, marked.parse(await response.text()));
            } else {
                // Generate a warning if the fetch fails
                add_text(description_div, "[Unable to read description.md]");
            }
        } else if(name == "labels.msgpack") {
            // If labels.msgpack exists we unpack it to get a map of (filename, description) pairs
            const response = await fetch(labels_url, {responseType: "arraybuffer"});
            if(response.status == 200) {
                const buffer = await response.arrayBuffer();
                const unpacked = MessagePack.decode(buffer);
                labels = new Map(Object.entries(unpacked));
            } else {
                // Generate a warning if the fetch fails
                add_text(description_div, "[Unable to read labels.msgpack]");
            }
        } else {
            nr_displayed +=1;
        }
    }

    // Create list of subdirectories
    const nr_dirs = Object.keys(object.directories).length;
    if(nr_dirs > 0) {
        const dir_header = add_element(div, "h3");
        add_text(dir_header, "Directories:");
        const dir_table = add_element(div, "table");
        for(let i=0; i<nr_dirs; i++) {
            const tr = add_element(dir_table, "tr");
            const name = Object.keys(object.directories)[i];
            const td1 = add_element(tr, "td");
            const dir_link = add_element(td1, "a");
            dir_link.href = viewer_url(join_path(path, name));
            add_text(dir_link, name);
            dir_link.onclick = function() {display_path(path+"/"+name, null, true) ; return false}; // returning false prevents loading the href
            // Add description of this directory, if we have one
            const td2 = add_element(tr, "td");
            if(labels != null) {
                if(labels.has(name)) {
                    // Labels are interpreted as inline fragments of markdown
                    const md = marked.parseInline(labels.get(name));
                    set_inner_html(td2, md);
                }
            }
        }
    }

    // Create list of files
    const file_header = add_element(div, "h3");
    if(nr_files > 0) {
        const file_table = add_element(div, "table");
        for(let i=0; i<nr_files; i++) {
            const name = Object.keys(object.files)[i];
            if((name != "description.md") && (name != "labels.msgpack")) {
                // This is a regular data file, so display it
                const tr = add_element(file_table, "tr");
                const td1 = add_element(tr, "td");
                const file_link = add_element(td1, "a");
                file_link.href = viewer_url(join_path(path, name));
                add_text(file_link, name);
                file_link.onclick = function() {display_path(path+"/"+name, null, true) ; return false}; // returning false prevents loading the href
                // Add description of this directory, if we have one
                const td2 = add_element(tr, "td");
                if(labels != null) {
                    if(labels.has(name)) {
                        add_text(td2, labels.get(name));
                    }
                }
            }
        }
    }
    if(nr_displayed > 0)add_text(file_header, "Files:");

    return frag;
}

// Function called when a details element representing a HDF5 group is opened
function on_details_opened(event) {

    const details = event.target;
    const root_name = event.target.dataset.root_name;
    const path = event.target.dataset.path;
    display_hdf5_group(path, root_name).then((group) => {
        details.appendChild(group);
    });
}

// Add attributes of object to the specified list
function append_attributes(list, root) {

    const nr_attrs = Object.keys(root.attributes).length;
    for(let i=0; i<nr_attrs; i++) {
        const name = Object.keys(root.attributes)[i];
        const object = Object.values(root.attributes)[i];
        const li = add_element(list, "li");
        const b = add_element(li, "b");
        add_text(b, name);
        add_text(li, " : " + format_attribute_value(object));
    }
}

//
// Here we implement lazy loading of group listings
//
// path is the virtual path to a file
// root_name is the name of the group to display
//
// Sub-groups are represented as html details tags which trigger a request
// to the server to fetch their contents on opening.
//
async function display_hdf5_group(path, root_name) {

    // Open the specified group
    const root = await fetch_and_decode(path, root_name);

    // Will return an unordered list of group members
    const group_list = document.createElement("ul");

    // Add any attributes of the group to the list first
    append_attributes(group_list, root);

    // Loop over groups in group 'root' and add them to the list
    const nr_members = Object.keys(root.members).length;
    for(let i=0; i<nr_members; i++) {
        const name = Object.keys(root.members)[i];
        const object = Object.values(root.members)[i];

        // Add a list item, if this is a group
        if((object == null) || (object.hdf5_object == "group")) {

            // Create list item for this group
            const li = add_element(group_list, "li");

            // Add the expandable details/summary box
            const details = add_element(li, "details");
            const summary = add_element(details, "summary");
            add_text(summary, name);

            // Store the file path and group name in the details element
            details.dataset.path = path;
            details.dataset.root_name = join_path(root_name, name);

            // On opening the details box we need to load the group's contents
            details.addEventListener("toggle", on_details_opened, {once : true});

        } else if(object.hdf5_object == "soft_link") {

            // Handle the case where we have a soft link
            const li = add_element(group_list, "li");
            const details = add_element(li, "details");
            const summary = add_element(details, "summary");
            add_text(summary, name);
            add_text(details, "  Soft link to "+object.target);
        }
    }

    // Now add any datasets in the group to the list
    for(let i=0; i<nr_members; i++) {
        const name = Object.keys(root.members)[i];
        const object = Object.values(root.members)[i];
        if((object != null) && (object.hdf5_object == "dataset")) {
            // Create list item for this dataset
            const li = add_element(group_list, "li");
            const details = add_element(li, "details");
            const summary = add_element(details, "summary");
            // Add the dataset name as a link
            const ds_link = add_element(summary, "a");
            const ds_name = join_path(root_name, name);
            ds_link.href = viewer_url(path, ds_name);
            ds_link.onclick = function() {display_path(path, ds_name, true) ; return false}; // returning false prevents loading the href
            add_text(ds_link, name);
            // Add type and dimensions information
            const ul = add_element(details, "ul");
            const li1 = add_element(ul, "li");
            const b1 = add_element(li1, "b");
            add_text(b1, "Data type");
            add_text(li1, " : "+object.type);
            const li2 = add_element(ul, "li");
            const b2 = add_element(li2, "b");
            add_text(b2, "Shape");
            add_text(li2, " : "+format_shape(object.shape));

            // Add any dataset attributes to the list
            append_attributes(ul, object);
        }
    }

    return group_list;
}

async function display_hdf5_file(path, object) {

    const frag = document.createDocumentFragment();
    let div = add_element(frag, "div");
    div.classList.add("hdf5_listing");

    // Create header with the file name
    add_filename_header(div, path, object.size, false);

    // Show how to access this dataset in python
    div.appendChild(python_hdf5_file_example(path));

    // Add header
    const header = add_element(div, "h3");
    add_text(header, "HDF5 file contents:");

    // Display the root group in the file
    div.appendChild(await display_hdf5_group(path, "/"))

    return frag;
}

async function display_text_file(path, object, file_type) {

    const frag = document.createDocumentFragment();
    const div = add_element(frag, "div");

    // Create header with the file name
    add_filename_header(div, path, object.size, false);

    // Download file contents
    let url = download_url(path);
    const response = await fetch(url);
    const text_content = await response.text();

    // Add a pre tag with the contents
    const pre = add_element(div, "pre");
    const code = add_element(pre, "code");
    add_text(code, text_content);

    // We can syntax highlight some langauges
    switch(file_type) {
    case "application/yaml":
        code.classList.add("language-yaml");
        hljs.highlightElement(code);
        break;
    case "text/plain":
        code.classList.add("language-plaintext");
        hljs.highlightElement(code);
        break;
    }

    return frag;
}

async function display_markdown_file(path, object) {

    const frag = document.createDocumentFragment();
    const div = add_element(frag, "div");

    // Create header with the file name
    add_filename_header(div, path, object.size, false);

    // Download file contents
    let url = download_url(path);
    const response = await fetch(url);
    const text_content = await response.text();

    // Render the markdown to html then sanitize and parse it
    const content_div = add_element(div, "div");
    set_inner_html(content_div, marked.parse(text_content));

    return frag;
}

async function display_html_file(path, object) {

    const frag = document.createDocumentFragment();
    const div = add_element(frag, "div");

    // Create header with the file name
    add_filename_header(div, path, object.size, false);

    // Download file contents
    let url = download_url(path);
    const response = await fetch(url);
    const text_content = await response.text();

    // Sanitize and parse the html
    const content_div = add_element(div, "div");
    set_inner_html(content_div, text_content);

    return frag;
}

function display_binary_file(path, object) {

    const frag = document.createDocumentFragment();
    const div = add_element(frag, "div");

    // Create header with the file name
    add_filename_header(div, path, object.size, false);

    // Add description
    const p = add_element(div, "p");
    add_text(p, "Unable to display this file type. Use the link above to download the file.");

    return frag;
}

// Class to display contents of a dataset as a html table
class DataTable {

    constructor(path, object_name, object_data) {

        this.path = path;
        this.object_name = object_name;

        // Maximum rows to display
        this.max_length = 100;

        // Maximum columns to display
        this.max_cols = 20;

        // Find shape of the dataset
        this.shape = object_data.shape;
        this.rank = object_data.shape.length;

        // Compute number of rows we have to display
        this.nr_rows = 1;
        if(this.rank > 0)this.nr_rows = this.shape[0];

        // Compute number of columns we have to display
        this.nr_cols = 1;
        for(let i=1; i<this.rank; i++)
            this.nr_cols *= this.shape[i];

        // Initial view settings
        this.view_offset = 0;
        this.view_length = this.max_length;
    }

    async view(offset=null, length=null) {

        if(offset == null)
            offset = this.view_offset;
        if(length == null)
            length = this.view_length;

        // Will return a div containing the content
        const result = document.createElement("div");
        result.id = "data_table_div";

        // Check if we can display the data
        if(this.rank > 2) {
            add_text(result, "Too many dimensions!");
            return result;
        }
        if(this.nr_cols > this.max_cols) {
            add_text(result, "Too many dimensions!");
            return result;
        }

        // Determine range of rows to display
        let row_offset = offset;
        if(row_offset < 0)row_offset = 0;
        if(row_offset >= this.nr_rows)row_offset = this.nr_rows-1;
        let row_count = length;
        if(row_count < 1)row_count = 1;
        if(row_count > this.nr_rows-row_offset)row_count = this.nr_rows-row_offset;

        // Request the data for the displayed part of the dataset
        const start = Array(this.rank);
        const count = Array(this.rank);
        for(var i=0; i<this.rank; i++) {
            start[i] = 0;
            count[i] = this.shape[i]
        }
        if(this.rank > 0) {
            start[0] = row_offset;
            count[0] = row_count;
        }
        const slice_data = await fetch_and_decode_slice(this.path, this.object_name, start, count);

        // Make a table
        const table = add_element(result, "table");
        table.classList.add("data_table");
        for(let i=0; i<row_count; i++) {
            const tr = add_element(table, "tr");
            // Add row index as first column in the row
            const index = add_element(tr, "td");
            add_text(index, (row_offset+i).toString());
            // Add data entries for this row
            for(let j=0; j<this.nr_cols; j++) {
                const td = add_element(tr, "td");
                const data_offset = (i*this.nr_cols) + j;
                const text = format_ndarray_element(slice_data, data_offset);
                add_text(td, text);
            }
        }

        // Update view parameters
        this.view_offset = row_offset;
        //this.view_length = row_count;

        return result;
    }
}

function python_dataset_example(path, object_name) {

    const result = document.createElement("div");

    // Get the server URL, which we'll need to construct the example code.
    // This is the page URL up to (but excluding) the /hdfstream/viewer part.
    const url = new URL(window.location.href);
    const server = url.origin; // E.g. https://localhost:8443

    // Split into root directory and rest of path
    const dir_name = path.split("/", 1)[0];
    const file_name = sanitize_path(path.replace(dir_name+"/", ""));

    // Add a header
    add_text(add_element(result, "h3"), "Read this dataset in python");

    // Put example code in pre and code tags
    const pre = add_element(result, "pre")
    const code = add_element(pre, "code")
    code.classList.add("language-python");

    // Construct the example text
    let example = "";
    example += 'import hdfstream\n';
    example += 'root_dir = hdfstream.open("'+server+'/hdfstream/", "'+dir_name+'")\n';
    example += 'h5file = root_dir["'+file_name+'"]\n';
    example += 'data = h5file["'+sanitize_path(object_name)+'"][...]\n';
    add_text(code, example);
    hljs.highlightElement(code);

    return result;
}


function python_hdf5_file_example(path) {

    const result = document.createElement("div");

    // Get the server URL, which we'll need to construct the example code.
    // This is the page URL up to (but excluding) the /hdfstream/viewer part.
    const url = new URL(window.location.href);
    const server = url.origin; // E.g. https://localhost:8443

    // Split into root directory and rest of path
    const dir_name = path.split("/", 1)[0];
    const file_name = sanitize_path(path.replace(dir_name+"/", ""));

    // Add a header
    add_text(add_element(result, "h3"), "Open this file in python");

    // Put example code in pre and code tags
    const pre = add_element(result, "pre")
    const code = add_element(pre, "code")
    code.classList.add("language-python");

    // Construct the example text
    let example = "";
    example += 'import hdfstream\n';
    example += 'root_dir = hdfstream.open("'+server+'/hdfstream/", "'+dir_name+'")\n';
    example += 'h5file = root_dir["'+file_name+'"]\n';
    add_text(code, example);
    hljs.highlightElement(code);

    return result;
}


async function display_dataset(path, file_data, object_name, object_data) {

    const frag = document.createDocumentFragment();
    const div = add_element(frag, "div");

    // Create header with the file name
    add_filename_header(div, path, file_data.size, true);

    // Add header with dataset name
    const name_header = add_element(div, "h2");
    add_text(name_header, "Dataset: "+object_name);

    // Show how to access this dataset in python
    div.appendChild(python_dataset_example(path, object_name));

    // Add attributes, if there are any
    const nr_attrs = Object.keys(object_data.attributes).length;
    if(nr_attrs > 0) {
        const attributes_header = add_element(div, "h3");
        add_text(attributes_header, "Attributes");
        const attributes_list = add_element(div, "ul");
        append_attributes(attributes_list, object_data);
    }

    // Add header for contents
    const contents_header = add_element(div, "h3");
    add_text(contents_header, "Dataset contents");

    // Add type and dimensions information
    const contents_list = add_element(div, "ul");
    const li1 = add_element(contents_list, "li");
    add_text(li1, "Data type : "+object_data.type);
    const li2 = add_element(contents_list, "li");
    add_text(li2, "Shape : "+format_shape(object_data.shape));

    // Add control for offset into the dataset
    const input_label = add_element(div, "label")
    add_text(input_label, "First row shown : ")
    const input_offset = add_element(input_label, "input")
    input_offset.type = "number";
    input_offset.value = 0;

    // Display dataset contents
    const data_table = new DataTable(path, object_name, object_data);
    const table_element = await data_table.view();
    div.appendChild(table_element);

    // Reload the table if the offset changes
    input_offset.addEventListener("change", (event) => {
        const offset = parseInt(input_offset.value, 10);
        data_table.view(offset).then((new_table) => {
            const old_table = document.getElementById("data_table_div");
            old_table.replaceWith(new_table);
        });
    });

    return frag;
}

// Class to store page state for browser history and generate associated paths
class ViewerState {

    constructor(url, path=null, object=null, page=null) {
        if(url != null) {
            // Set state from the supplied URL
            this.init_from_url(url);
        } else {
            // Set state from (path, object, page)
            this.path = sanitize_path(path);
            this.object = object;
            this.page = page;
            this.url = viewer_url(this.path, this.object, this.page).toString();
        }
    }

    // Update this state object from the supplied URL
    init_from_url(url) {
        // Extract virtual path from the URL
        let path = decode_path(url.pathname);
        // Extract object name from the URL
        let object = url.searchParams.get("object");
        if(object != null)
            object = "/"+sanitize_path(decodeURIComponent(object));
        // Extract any static page name from the URL
        let page = url.searchParams.get("page");
        // Remove prefix from the path
        if(path.startsWith("/hdfstream/viewer")) {
            path = path.slice("/hdfstream/viewer".length);
        } else {
            // If we have an unrecognized URL format, go to the overview page
            path = null;
            object = null;
            page = "overview";
        }
        // Store parameters
        this.path = sanitize_path(path);
        this.object = object;
        this.page = page;
        // Generate sanitized URL string
        this.url = viewer_url(this.path, this.object, this.page).toString();
    }
}

async function display_path(path, object_name, push_state) {
    //
    // Function to display a virtual path
    //
    // Downloads the msgpack description of the object and then uses DOM API
    // calls to generate a html view.
    //
    // Pushes path to the browser history if push_state=true.
    //

    let result = null;
    try {

        // Download the file or directory object for this path
        const file_data = await fetch_and_decode(path);

        // Generate html description
        const file_type = file_data.type.split(";")[0];
        if(object_name == null) {
            // We're displaying a file or directory
            switch(file_type) {
            case "directory":
                result = await display_directory(path, file_data);
                break;
            case "application/x-hdf5":
                result = await display_hdf5_file(path, file_data);
                break;
            case "application/yaml":
            case "text/plain":
                result = await display_text_file(path, file_data, file_type);
                break;
            case "text/markdown":
                result = await display_markdown_file(path, file_data);
                break;
            case "text/html":
                result = await display_html_file(path, file_data);
                break;
            case "application/octet-stream":
            default:
                // Unrecognized file types are treated as binary data
                result = display_binary_file(path, file_data);
                break;
            }
        } else {
            // We're displaying a dataset. Download the dataset object too.
            const object_data = await fetch_and_decode(path, object_name);
            result = await display_dataset(path, file_data, object_name, object_data);
        }

    } catch (err) {

        // Something went wrong, so display the error
        result = document.createElement("div");
        const p = add_element(result, "p");
        add_text(p, err.message);

    }

    // Update the page
    display_content(result);

    if(push_state) {
        // Set state used for browser history and back button
        const state = new ViewerState(null, path, object_name);
        window.history.pushState(state, null, state.url);
    }
}

async function display_static_page(page, push_state) {

    let result = null;
    try {
        // Check for a valid page name
        switch(page) {
        case "overview":
        case "web_interface":
        case "python_module":
        case "python_examples":
        case "performance":
        case "api_description":
        case "msgpack_encoding":
            break;
        default:
            throw new Error("Invalid page name");
            break;
        }
        // Fetch the page content
        const response = await fetch(docs_url(page));
        if (!response.ok) {
          throw new Error('Unable to fetch static page');
        }
        const content = await response.text();
        result = document.createElement("div");
        set_inner_html(result, content);

    } catch (err) {

        // Something went wrong, so display the error
        result = document.createElement("div");
        const p = add_element(result, "p");
        add_text(p, err.message);

    }

    // Substitute in the server URL in examples where needed:
    // If we see __SERVER_URL__ in any code tags, replace it.
    const server_url = new URL("/hdfstream/", window.location.origin).toString();
    const code_blocks = result.querySelectorAll('code');
    code_blocks.forEach(block => {
        block.textContent = block.textContent.replaceAll("__SERVER_URL__", server_url);
    });

    // Update the page
    display_content(result);

    // Apply syntax highlighting
    hljs.highlightAll();

    if(push_state) {
        // Set state used for browser history and back button
        const state = new ViewerState(null, null, null, page);
        window.history.pushState(state, null, state.url);
    }
}

// Given a ViewerState instance, update the display
function display_state(state, push_state=true) {

    if(state.page != null) {
        // If page parameter is set, fetch and display static content
        display_static_page(state.page, push_state);
    } else {
        // Otherwise we're displaying a virtual path
        display_path(state.path, state.object, push_state);
    }
}

// Main function called on page load only
export function viewer_onload() {

    // Viewer state is fully determined by the page URL
    const state = new ViewerState(new URL(window.location.href));

    // Set initial state. We "fake" browser history here by calling pushState
    // when we navigate to new file or directory. The state.url field
    // returns the URL associated with the new page.
    window.history.replaceState(state, null, state.url);

    // Add the collection list:
    // This shows all directories this user can access
    const collections_list = document.createElement("ul");
    const collections_item = add_element(collections_list, "li");
    const collections_link = add_element(collections_item, "a");
    update_collection_list();

    // Add documentation links:
    // These have a valid URL which can be linked to from elsewhere, but we
    // prefer to fetch the content without a full page reload when the link is
    // clicked.
    const docs_list = document.getElementById("service_docs");
    docs_list.appendChild(make_docs_link_item("overview", "Overview"));
    docs_list.appendChild(make_docs_link_item("web_interface", "Web interface"));
    docs_list.appendChild(make_docs_link_item("python_module", "Python module"));
    docs_list.appendChild(make_docs_link_item("python_examples", "Python examples"));
    docs_list.appendChild(make_docs_link_item("performance", "Performance tips"));
    const tech_list = document.getElementById("tech_docs");
    tech_list.appendChild(make_docs_link_item("api_description", "API description"));
    tech_list.appendChild(make_docs_link_item("msgpack_encoding", "Messagepack encoding"));

    // Update the display using state derived from the URL
    display_state(state, false);

    // When the back button is pressed we should update the display using
    // the path stored in the old state object.
    window.onpopstate = function (event) {
        if(event.state != null) {
            display_state(event.state, false);
        }
    };
}

// Generate a link to a static documentation page as a list item
function make_docs_link_item(page_name, text) {
    const li = document.createElement("li")
    const link = add_element(li, "a");
    add_text(link, text);
    link.href = viewer_url(null, null, page_name);
    link.onclick = function() {display_static_page(page_name, true) ; return false};
    return li;
}

// Replace the current collection list with the provided element
function display_collection_list(node) {

    const oldContent = document.getElementById("collection_list");
    const newContent = document.createElement("div");
    newContent.className = oldContent.className;
    newContent.id = oldContent.id;
    newContent.appendChild(node);
    oldContent.replaceWith(newContent);
}

// Show the list of top level directories which this user can access
async function update_collection_list() {

    // Fetch and decode the directory listing
    const data = await fetch_and_decode("");

    // Construct the list of collections
    const div = document.createElement("div");
    const ul = add_element(div, "ul");

    // Loop over directories and add list items
    const nr_dirs = Object.keys(data.directories).length;
    for(let i=0; i<nr_dirs; i++) {
        const name = Object.keys(data.directories)[i];
        const item = add_element(ul, "li");
        const link = add_element(item, "a");
        link.href = viewer_url(name);
        add_text(link, name);
        link.onclick = function() {display_path(name, null, true) ; return false};
    }

    // Display the list
    display_collection_list(div);
}
