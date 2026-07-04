import React from 'react'

export default function AssemblyViewer({ assembly, loading = false }) {
  const lines = assembly ? assembly.split('\n') : []

  return (
    <div style={{
      width: '100%',
      height: '100%',
      display: 'flex',
      flexDirection: 'column',
      background: '#1e1e1e',
    }}>
      <div style={{
        padding: '8px 16px',
        borderBottom: '1px solid #3e3e42',
        background: '#252526',
        fontSize: '13px',
        fontWeight: '500',
        color: '#cccccc',
      }}>
        output.s (x86-64)
      </div>

      <div style={{
        flex: 1,
        overflow: 'auto',
        padding: '16px',
        fontFamily: 'Fira Code',
        fontSize: '12px',
        lineHeight: '1.6',
        background: '#1e1e1e',
      }}>
        {loading ? (
          <div style={{ color: '#666' }}>Compilando...</div>
        ) : assembly ? (
          <pre style={{
            color: '#d4d4d4',
            margin: 0,
            whiteSpace: 'pre-wrap',
            wordWrap: 'break-word',
          }}>
            {lines.map((line, idx) => (
              <div key={idx} style={{
                display: 'flex',
                paddingRight: '16px',
              }}>
                <span style={{
                  color: '#858585',
                  marginRight: '16px',
                  minWidth: '40px',
                  textAlign: 'right',
                  userSelect: 'none',
                }}>
                  {idx + 1}
                </span>
                <span style={{
                  flex: 1,
                }}>
                  {line}
                </span>
              </div>
            ))}
          </pre>
        ) : (
          <div style={{ color: '#666' }}>
            Compila codigo para ver el assembly
          </div>
        )}
      </div>
    </div>
  )
}
