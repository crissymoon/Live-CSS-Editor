<?php
/**
 * CSS property groups, used to populate the header dropdown
 * and build the flat autocomplete list.
 */
$cssProperties = [
    'Layout' => [
        'display', 'position', 'top', 'right', 'bottom', 'left', 'float', 'clear',
        'z-index', 'overflow', 'overflow-x', 'overflow-y', 'visibility', 'opacity',
        'box-sizing', 'vertical-align',
    ],
    'Flexbox' => [
        'flex', 'flex-basis', 'flex-direction', 'flex-flow', 'flex-grow', 'flex-shrink',
        'flex-wrap', 'justify-content', 'align-items', 'align-content', 'align-self',
        'order', 'gap', 'row-gap', 'column-gap',
    ],
    'Grid' => [
        'grid', 'grid-template', 'grid-template-columns', 'grid-template-rows',
        'grid-template-areas', 'grid-column', 'grid-row', 'grid-area',
        'grid-auto-columns', 'grid-auto-rows', 'grid-auto-flow',
        'grid-column-start', 'grid-column-end', 'grid-row-start', 'grid-row-end',
        'place-items', 'place-content', 'place-self',
    ],
    'Sizing' => [
        'width', 'height', 'min-width', 'min-height', 'max-width', 'max-height',
        'aspect-ratio',
    ],
    'Spacing' => [
        'margin', 'margin-top', 'margin-right', 'margin-bottom', 'margin-left',
        'padding', 'padding-top', 'padding-right', 'padding-bottom', 'padding-left',
    ],
    'Border' => [
        'border', 'border-top', 'border-right', 'border-bottom', 'border-left',
        'border-width', 'border-style', 'border-color',
        'border-top-width', 'border-right-width', 'border-bottom-width', 'border-left-width',
        'border-top-style', 'border-right-style', 'border-bottom-style', 'border-left-style',
        'border-top-color', 'border-right-color', 'border-bottom-color', 'border-left-color',
        'border-collapse', 'border-spacing', 'border-image',
        'outline', 'outline-width', 'outline-style', 'outline-color', 'outline-offset',
    ],
    'Background' => [
        'background', 'background-color', 'background-image', 'background-position',
        'background-size', 'background-repeat', 'background-attachment',
        'background-origin', 'background-clip', 'background-blend-mode',
    ],
    'Color' => [
        'color', 'opacity', 'filter', 'mix-blend-mode',
    ],
    'Typography' => [
        'font', 'font-family', 'font-size', 'font-weight', 'font-style', 'font-variant',
        'font-stretch', 'line-height', 'letter-spacing', 'word-spacing',
        'text-align', 'text-decoration', 'text-decoration-color', 'text-decoration-line',
        'text-decoration-style', 'text-transform', 'text-indent', 'text-shadow',
        'text-overflow', 'white-space', 'word-break', 'word-wrap', 'overflow-wrap',
        'writing-mode', 'direction', 'tab-size', 'hyphens',
    ],
    'List' => [
        'list-style', 'list-style-type', 'list-style-position', 'list-style-image',
    ],
    'Table' => [
        'table-layout', 'caption-side', 'empty-cells',
    ],
    'Transform' => [
        'transform', 'transform-origin', 'transform-style', 'perspective',
        'perspective-origin', 'backface-visibility', 'translate', 'rotate', 'scale',
    ],
    'Transition and Animation' => [
        'transition', 'transition-property', 'transition-duration',
        'transition-timing-function', 'transition-delay',
        'animation', 'animation-name', 'animation-duration',
        'animation-timing-function', 'animation-delay', 'animation-iteration-count',
        'animation-direction', 'animation-fill-mode', 'animation-play-state',
    ],
    'Misc' => [
        'cursor', 'pointer-events', 'user-select', 'resize', 'scroll-behavior',
        'scroll-snap-type', 'scroll-snap-align', 'object-fit', 'object-position',
        'clip-path', 'mask', 'will-change', 'content', 'counter-reset', 'counter-increment',
        'box-shadow', 'columns', 'column-count', 'column-gap', 'column-rule',
        'page-break-before', 'page-break-after', 'page-break-inside',
        'all', 'appearance', 'accent-color', 'caret-color', 'color-scheme',
    ],
];
