<?php
/**
 * Default HTML and CSS loaded into the editors on first visit or after reset.
 */
$defaultHtml = '<div class="container">
  <h1>Hello, World!</h1>
  <p>This is a live CSS editor.</p>
  <button class="btn">Click Me</button>
  <ul class="list">
    <li>Item One</li>
    <li>Item Two</li>
    <li>Item Three</li>
  </ul>
</div>';

$defaultCss = '.container {
  padding: 24px;
  font-family: sans-serif;
}

h1 {
  color: #2d1c6e;
  margin-bottom: 12px;
}

p {
  color: #1b1825;
  font-size: 18px;
  line-height: 1.6;
}

.btn {
  background: #2d1c6e;
  color: #eceaf6;
  border: 2px solid #4d31bf;
  padding: 10px 24px;
  font-size: 16px;
  cursor: pointer;
}

.btn:hover {
  background: #4d31bf;
}

.list {
  margin-top: 16px;
  padding-left: 20px;
}

.list li {
  padding: 4px 0;
  color: #1b1825;
}';

$defaultJs = '// JavaScript runs in the live preview\ndocument.addEventListener("DOMContentLoaded", function () {\n  var btn = document.querySelector(".btn");\n  if (btn) {\n    btn.addEventListener("click", function () {\n      alert("Button clicked!");\n    });\n  }\n});';
