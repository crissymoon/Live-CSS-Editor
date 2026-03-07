// webview_js.h -- JavaScript strings injected by the WKWebView bridge.
// Included by webview_core.mm and webview_delegates.mm.
// All strings are static within each translation unit (Obj-C string literals
// are pointer-deduplicated by the linker so there is no actual duplication).
#pragma once

#import <Foundation/Foundation.h>

// ── FPS probe ─────────────────────────────────────────────────────────
// Read page-side stats-inject metrics.  Evaluated every 2 s per tab.
static NSString* const JS_FPS_PROBE = @"(function(){"
    "var s=window.__xcmStats||window.__xcm&&window.__xcm.stats;"
    "var fps=s&&typeof s.fps==='function'?Math.round(+s.fps()||0):0;"
    "var hz=window.__xcmHz||0;"
    "return [fps,hz];"
    "})()";

// ── Document-start init ───────────────────────────────────────────────
// Injects host flags and the image-proxy base URL before any page JS runs.
static NSString* const JS_INIT = @"(function(){"
    "window.__xcmImguiHost=true;"
    "window.__xcmImgProxy='http://127.0.0.1:7779';"
    "})();";

// ── Fingerprint noise ─────────────────────────────────────────────────
// Adds human-like hardware variation so automated fingerprinting cannot
// identify this as a headless / bot client.
// Covered: Canvas 2D, WebGL vendor/renderer, AudioBuffer, hardwareConcurrency,
//          navigator.platform, outerWidth/outerHeight.
// NOTE: JS_MASK_WEBVIEW was removed -- the masks patched APIs that Cloudflare
// Turnstile probes via Object.getOwnPropertyDescriptor.  WKWebView's native
// Safari properties are already correct without patching.
static NSString* const JS_FINGERPRINT_NOISE = @"(function(){"

    "var _s=(Date.now()^(Math.random()*0x7FFFFFFF))|1;"
    "function _r(){"
    "  _s=(_s^(_s<<13)|0)>>>0;"
    "  _s=(_s^(_s>>>17))>>>0;"
    "  _s=(_s^(_s<<5)|0)>>>0;"
    "  return _s;"
    "}"
    "function _r2(){return _r()&1?1:-1;}"

    "try{"
    "  var _ogc=HTMLCanvasElement.prototype.getContext;"
    "  HTMLCanvasElement.prototype.getContext=function(t,o){"
    "    var ctx=_ogc.call(this,t,o);"
    "    if(ctx&&t==='2d'&&ctx.getImageData){"
    "      var _og=ctx.getImageData.bind(ctx);"
    "      ctx.getImageData=function(x,y,w,h){"
    "        var d=_og(x,y,w,h);"
    "        if(d&&d.data&&d.data.length>16){"
    "          for(var i=0;i<3;i++){"
    "            var idx=(_r()%(d.data.length>>>2))<<2;"
    "            d.data[idx]=(d.data[idx]+_r2()+256)&255;"
    "          }"
    "        }"
    "        return d;"
    "      };"
    "    }"
    "    return ctx;"
    "  };"
    "}catch(e){}"

    "var _WGLV=0x9245,_WGLR=0x9246;"
    "function _patchWGL(P){"
    "  try{"
    "    var og=P.getParameter.bind(P.__proto__||P);"
    "    P.constructor.prototype.getParameter=function(param){"
    "      if(param===_WGLV)return'Intel Inc.';"
    "      if(param===_WGLR)return'Intel Iris OpenGL Engine';"
    "      return og.call(this,param);"
    "    };"
    "  }catch(e){}"
    "}"
    "try{"
    "  var _c2=document.createElement('canvas');"
    "  var _g=_c2.getContext('webgl')||_c2.getContext('experimental-webgl');"
    "  if(_g)_patchWGL(_g);"
    "  var _g2=_c2.getContext('webgl2');"
    "  if(_g2)_patchWGL(_g2);"
    "}catch(e){}"

    "try{"
    "  var _oab=AudioBuffer.prototype.getChannelData;"
    "  AudioBuffer.prototype.getChannelData=function(ch){"
    "    var d=_oab.call(this,ch);"
    "    if(d&&d.length>4){"
    "      var i1=_r()%d.length,i2=_r()%d.length;"
    "      d[i1]+=_r2()*5e-8;"
    "      d[i2]+=_r2()*5e-8;"
    "    }"
    "    return d;"
    "  };"
    "}catch(e){}"

    "try{"
    "  if(!navigator.hardwareConcurrency||navigator.hardwareConcurrency<2){"
    "    Object.defineProperty(navigator,'hardwareConcurrency',{"
    "      get:function(){return 8;},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    "try{"
    "  if(navigator.platform!=='MacIntel'){"
    "    Object.defineProperty(navigator,'platform',{"
    "      get:function(){return'MacIntel';},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    "try{"
    "  if(!window.outerWidth||window.outerWidth===0){"
    "    Object.defineProperty(window,'outerWidth',{"
    "      get:function(){return window.innerWidth||1280;},"
    "      configurable:true"
    "    });"
    "  }"
    "  if(!window.outerHeight||window.outerHeight===0){"
    "    Object.defineProperty(window,'outerHeight',{"
    "      get:function(){return (window.innerHeight||800)+100;},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    "})();";

// ── Reporting API polyfill ────────────────────────────────────────────
// Captures CSP violation DOM events and POSTs them to the endpoint from
// the page's Report-To / Reporting-Endpoints header.
// The native side calls window.__xcmSetReportTo(endpointURL) from
// didFinishNavigation after reading and caching the header value.
// Injected into the main frame only to avoid duplicate reports from iframes.
static NSString* const JS_REPORT_TO_RELAY = @"(function(){"
    "var _ep=null,_q=[];"
    "function _send(r){"
    "  if(!_ep){_q.push(r);return;}"
    "  try{"
    "    fetch(_ep,{"
    "      method:'POST',"
    "      headers:{'Content-Type':'application/reports+json'},"
    "      body:JSON.stringify([r]),"
    "      keepalive:true,"
    "      credentials:'omit'"
    "    }).catch(function(){});"
    "  }catch(e){}"
    "}"
    "function _flush(){var q=_q.splice(0);for(var i=0;i<q.length;i++)_send(q[i]);}"
    "document.addEventListener('securitypolicyviolation',function(e){"
    "  _send({"
    "    type:'csp-violation',age:0,url:e.documentURI,user_agent:navigator.userAgent,"
    "    body:{"
    "      blockedURL:e.blockedURI,columnNumber:e.columnNumber,"
    "      disposition:e.disposition,documentURL:e.documentURI,"
    "      effectiveDirective:e.effectiveDirective,lineNumber:e.lineNumber,"
    "      originalPolicy:e.originalPolicy,referrer:e.referrer,"
    "      sample:e.sample||'',sourceFile:e.sourceFile,"
    "      statusCode:e.statusCode,violatedDirective:e.violatedDirective"
    "    }"
    "  });"
    "},true);"
    "window.__xcmSetReportTo=function(ep){_ep=ep;_flush();};"
    "})();";
