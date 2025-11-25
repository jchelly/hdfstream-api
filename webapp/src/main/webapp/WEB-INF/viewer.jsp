<!DOCTYPE html>
<html>

<head>
  
  <link rel="stylesheet" href="/hdfstream/common.css">
  <script crossorigin src="https://unpkg.com/@msgpack/msgpack@2.8.0"></script>
  <script crossorigin src="https://unpkg.com/dompurify@3.2.3/dist/purify.min.js"></script>

  <link rel="stylesheet" href="https://unpkg.com/@highlightjs/cdn-assets@11.11.1/styles/default.min.css">
  <script src="https://unpkg.com/@highlightjs/cdn-assets@11.11.1/highlight.min.js"></script>
  <script src="https://unpkg.com/@highlightjs/cdn-assets@11.11.1/languages/python.min.js"></script>
  <script src="https://unpkg.com/@highlightjs/cdn-assets@11.11.1/languages/yaml.min.js"></script>
  
</head>

<body>

  <div class="sidebar">
    
    <h3>Data collections</h3>
    <div id="collection_list"></div>

    <h3>Service documentation</h3>
    <ul id="service_docs"></ul>
    
    <h3>Technical details</h3>
    <ul id="tech_docs"></ul>

  </div>

  <div class="body-text">
    <div class="viewer" id="content">
      <!-- Javascript generated content goes here -->
      <noscript>This page requires javascript to work.</noscript>
    </div>  
  </div>

  <script type="module">
    import { viewer_onload } from "/hdfstream/viewer.js";
    window.addEventListener('load', viewer_onload);
  </script>

</body>
</html>
