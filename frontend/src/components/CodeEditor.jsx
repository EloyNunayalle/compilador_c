import React from 'react'
import Editor from '@monaco-editor/react'

export default function CodeEditor({ code, onChange, theme = 'dark' }) {
  return (
    <div style={{
      width: '100%',
      height: '100%',
      display: 'flex',
      flexDirection: 'column',
    }}>
      <div style={{
        padding: '8px 16px',
        borderBottom: '1px solid #3e3e42',
        background: '#252526',
        fontSize: '13px',
        fontWeight: '500',
        color: '#cccccc',
      }}>
        main.c
      </div>

      <Editor
        height="100%"
        defaultLanguage="c"
        value={code}
        onChange={onChange}
        theme={theme === 'dark' ? 'vs-dark' : 'vs'}
        options={{
          minimap: { enabled: false },
          fontSize: 14,
          fontFamily: 'Fira Code',
          lineHeight: 22,
          scrollBeyondLastLine: false,
          wordWrap: 'on',
          formatOnPaste: true,
          formatOnType: true,
          tabSize: 4,
          insertSpaces: true,
          smoothScrolling: true,
          renderLineHighlight: 'gutter',
          automaticLayout: true,
        }}
        defaultValue={code}
      />
    </div>
  )
}
