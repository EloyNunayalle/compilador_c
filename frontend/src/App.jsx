import React, { useState, useRef, useEffect, useCallback } from 'react'
import CodeEditor from './components/CodeEditor'
import ASTVisualizer from './components/ASTVisualizer'
import AssemblyViewer from './components/AssemblyViewer'
import OutputPanel from './components/OutputPanel'
import { compileCode } from './services/api'

const DEFAULT_CODE = `#include <stdio.h>

int main() {
    printf("Hola Mundo\\n");
    return 0;
}
`

function ResizablePanel({ topLeft, topRight, bottomLeft, bottomRight, initialCol = 50, initialRow = 50 }) {
  const [colRatio, setColRatio] = useState(initialCol)
  const [rowRatio, setRowRatio] = useState(initialRow)
  const dragging = useRef(null)
  const containerRef = useRef(null)

  const onMouseDown = useCallback((dir) => (e) => {
    e.preventDefault()
    dragging.current = dir
    document.body.style.cursor = dir === 'col' ? 'col-resize' : 'row-resize'
    document.body.style.userSelect = 'none'
  }, [])

  useEffect(() => {
    const onMouseMove = (e) => {
      if (!dragging.current || !containerRef.current) return
      const rect = containerRef.current.getBoundingClientRect()
      if (dragging.current === 'col') {
        const pct = ((e.clientX - rect.left) / rect.width) * 100
        setColRatio(Math.max(10, Math.min(90, pct)))
      } else {
        const pct = ((e.clientY - rect.top) / rect.height) * 100
        setRowRatio(Math.max(10, Math.min(90, pct)))
      }
    }
    const onMouseUp = () => {
      if (dragging.current) {
        dragging.current = null
        document.body.style.cursor = ''
        document.body.style.userSelect = ''
      }
    }
    window.addEventListener('mousemove', onMouseMove)
    window.addEventListener('mouseup', onMouseUp)
    return () => {
      window.removeEventListener('mousemove', onMouseMove)
      window.removeEventListener('mouseup', onMouseUp)
    }
  }, [])

  const dividerStyle = {
    background: '#3e3e42',
    flexShrink: 0,
    zIndex: 10,
  }

  const panelStyle = {
    overflow: 'hidden',
    display: 'flex',
    flexDirection: 'column',
  }

  return (
    <div ref={containerRef} style={{ display: 'flex', flexDirection: 'column', flex: 1, overflow: 'hidden' }}>
      <div style={{ display: 'flex', flex: rowRatio, overflow: 'hidden' }}>
        <div style={{ ...panelStyle, flex: colRatio }}>
          {topLeft}
        </div>
        <div
          style={{ ...dividerStyle, width: '5px', cursor: 'col-resize' }}
          onMouseDown={onMouseDown('col')}
        />
        <div style={{ ...panelStyle, flex: 100 - colRatio }}>
          {topRight}
        </div>
      </div>
      <div
        style={{ ...dividerStyle, height: '5px', cursor: 'row-resize' }}
        onMouseDown={onMouseDown('row')}
      />
      <div style={{ display: 'flex', flex: 100 - rowRatio, overflow: 'hidden' }}>
        <div style={{ ...panelStyle, flex: colRatio }}>
          {bottomLeft}
        </div>
        <div
          style={{ ...dividerStyle, width: '5px', cursor: 'col-resize' }}
          onMouseDown={onMouseDown('col')}
        />
        <div style={{ ...panelStyle, flex: 100 - colRatio }}>
          {bottomRight}
        </div>
      </div>
    </div>
  )
}

export default function App() {
  const [code, setCode] = useState(DEFAULT_CODE)
  const [ast, setAst] = useState(null)
  const [assembly, setAssembly] = useState(null)
  const [output, setOutput] = useState(null)
  const [error, setError] = useState(null)
  const [isCompiling, setIsCompiling] = useState(false)
  const [optimize, setOptimize] = useState(true)

  const handleCompile = async () => {
    if (!code.trim()) {
      setError('El codigo no puede estar vacio')
      return
    }

    setIsCompiling(true)
    setError(null)
    setOutput(null)

    try {
      const result = await compileCode(code, 'both', optimize)

      if (result.success) {
        setAst(result.ast)
        setAssembly(result.assembly)
        setError(null)
        setOutput(result.output !== undefined ? result.output :
                  result.assembly || 'Compilacion exitosa')
      } else {
        setError(result.error || 'Error de compilacion')
        setAst(null)
        setAssembly(null)
      }
    } catch (err) {
      setError('Error conectando con el compilador: ' + err.message)
      setAst(null)
      setAssembly(null)
    } finally {
      setIsCompiling(false)
    }
  }

  const handleReset = () => {
    setCode(DEFAULT_CODE)
    setAst(null)
    setAssembly(null)
    setError(null)
    setOutput(null)
  }

  return (
    <div style={{
      display: 'flex',
      flexDirection: 'column',
      height: '100vh',
      background: '#1e1e1e',
      color: '#e0e0e0',
    }}>
      {/* Toolbar */}
      <div style={{
        display: 'flex',
        alignItems: 'center',
        padding: '12px 16px',
        background: '#2d2d30',
        borderBottom: '1px solid #3e3e42',
        gap: '12px',
      }}>
        <div style={{
          fontSize: '16px',
          fontWeight: 'bold',
          marginRight: 'auto',
        }}>
          MiniC Compiler IDE
        </div>

        <label style={{
          display: 'flex',
          alignItems: 'center',
          gap: '6px',
          fontSize: '13px',
          cursor: 'pointer',
          userSelect: 'none',
        }}>
          <input
            type="checkbox"
            checked={optimize}
            onChange={(e) => setOptimize(e.target.checked)}
            style={{ cursor: 'pointer' }}
          />
          Optimizar (-O1)
        </label>

        <button
          onClick={handleCompile}
          disabled={isCompiling}
          style={{
            padding: '6px 16px',
            background: isCompiling ? '#555555' : '#007acc',
            color: 'white',
            border: 'none',
            borderRadius: '3px',
            cursor: isCompiling ? 'not-allowed' : 'pointer',
            fontSize: '13px',
            fontWeight: '500',
            transition: 'background 0.2s',
          }}
          onMouseEnter={(e) => {
            if (!isCompiling) e.target.style.background = '#005a9e'
          }}
          onMouseLeave={(e) => {
            if (!isCompiling) e.target.style.background = '#007acc'
          }}
        >
          {isCompiling ? 'Compilando...' : 'Compilar (Ctrl+Enter)'}
        </button>

        <button
          onClick={handleReset}
          style={{
            padding: '6px 16px',
            background: '#555555',
            color: 'white',
            border: 'none',
            borderRadius: '3px',
            cursor: 'pointer',
            fontSize: '13px',
            fontWeight: '500',
            transition: 'background 0.2s',
          }}
          onMouseEnter={(e) => e.target.style.background = '#666666'}
          onMouseLeave={(e) => e.target.style.background = '#555555'}
        >
          Reiniciar
        </button>
      </div>

      {/* Main Layout with Resizable Panels */}
      <div style={{ flex: 1, display: 'flex', overflow: 'hidden' }}>
        <ResizablePanel
          topLeft={
            <CodeEditor
              code={code}
              onChange={setCode}
              theme="dark"
            />
          }
          topRight={
            <>
              <div style={{
                padding: '8px 16px',
                borderBottom: '1px solid #3e3e42',
                background: '#252526',
                fontSize: '13px',
                fontWeight: '500',
                color: '#cccccc',
                flexShrink: 0,
              }}>
                Abstract Syntax Tree (AST)
              </div>
              <ASTVisualizer ast={ast} loading={isCompiling} />
            </>
          }
          bottomLeft={
            <AssemblyViewer assembly={assembly} loading={isCompiling} />
          }
          bottomRight={
            <OutputPanel
              output={output}
              error={error}
              isCompiling={isCompiling}
            />
          }
        />
      </div>

      {/* Status Bar */}
      <div style={{
        padding: '4px 16px',
        background: '#252526',
        borderTop: '1px solid #3e3e42',
        fontSize: '12px',
        color: '#858585',
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        flexShrink: 0,
      }}>
        <div>
          Lineas: {code.split('\n').length} | Caracteres: {code.length}
        </div>
        <div>
          {isCompiling ? 'Compilando...' : 'Listo'}
        </div>
      </div>

      <KeyboardShortcuts onCompile={handleCompile} />
    </div>
  )
}

function KeyboardShortcuts({ onCompile }) {
  useEffect(() => {
    const handleKeyDown = (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        e.preventDefault()
        onCompile()
      }
    }

    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [onCompile])

  return null
}
