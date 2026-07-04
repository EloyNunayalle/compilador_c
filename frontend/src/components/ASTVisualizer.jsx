import React, { useEffect, useRef, useState } from 'react'
import * as d3 from 'd3'

function getNodeLabel(d) {
  const type = d.data.type || 'Node'
  switch (type) {
    case 'IntLit':
      return type + ': ' + (d.data.value ?? '')
    case 'FloatLit':
      return type + ': ' + (d.data.value ?? '')
    case 'StringLit':
      return type + ': "' + (d.data.value ?? '') + '"'
    case 'IdExp':
      return type + ': ' + (d.data.name ?? '')
    case 'FunDef':
      return type + ': ' + (d.data.name ?? '')
    case 'BinaryExp':
    case 'UnaryExp':
      return type + ' (' + (d.data.op ?? '') + ')'
    case 'AssignStm':
      return type + ' (' + (d.data.op ?? '=') + ')'
    case 'StructDef':
      const fnames = (d.data.fields || []).map(f => f.name).join(', ')
      return type + ': ' + (d.data.name ?? '') + ' {' + (fnames ? fnames : '') + '}'
    default:
      return type
  }
}

const NODE_RADIUS = 28

function getNodeRadius() {
  return NODE_RADIUS
}

function childrenAccessor(d) {
  switch (d.type) {
    case 'Program':
      return [...(d.structs || []), ...(d.globals || []), ...(d.functions || [])]
    case 'FunDef':
      return d.body ? [d.body] : []
    case 'Block':
      return d.statements
    case 'VarDecStm':
      return d.declarations?.flatMap((v) => v.init ? [v.init] : [])
    case 'AssignStm':
      return [d.lvalue, d.rvalue].filter(Boolean)
    case 'ExprStm':
      return d.expr ? [d.expr] : []
    case 'ReturnStm':
      return d.expr ? [d.expr] : []
    case 'IfStm':
      return d.else ? [d.condition, d.then, d.else] : [d.condition, d.then]
    case 'WhileStm':
    case 'DoWhileStm':
      return [d.condition, d.body].filter(Boolean)
    case 'ForStm':
      return [d.init, d.condition, d.update, d.body].filter(Boolean)
    case 'BinaryExp':
      return [d.left, d.right].filter(Boolean)
    case 'UnaryExp':
    case 'AddrExp':
    case 'DerefExp':
    case 'CastExp':
      return d.expr ? [d.expr] : []
    case 'CallExp':
      return d.args || []
    case 'IndexExp':
      return [d.base, d.index].filter(Boolean)
    case 'FieldExp':
      return d.expr ? [d.expr] : []
    case 'StructDef':
      return [] // fields tienen type como objeto Type, no como string discriminador
    default:
      return []
  }
}

export default function ASTVisualizer({ ast, loading = false }) {
  const containerRef = useRef(null)
  const svgRef = useRef(null)
  const [zoom, setZoom] = useState(1)

  useEffect(() => {
    if (!ast || !containerRef.current) return

    const container = containerRef.current
    const width = container.clientWidth
    const height = container.clientHeight

    const svg = d3.select(svgRef.current)
    svg.selectAll('*').remove()
    svg.attr('width', width).attr('height', height)

    const g = svg.append('g')

    const root = d3.hierarchy(ast, childrenAccessor)
    const allDescendants = root.descendants()

    const nodeW = NODE_RADIUS * 2 + 24

    const tree = d3.tree()
      .nodeSize([nodeW + 40, 90])
      .separation((a, b) => (NODE_RADIUS * 2 + 10) / nodeW)

    const treeData = tree(root)
    const allNodes = treeData.descendants()
    const allLinks = treeData.links()

    const minX = d3.min(allNodes, d => d.x) || 0
    const maxX = d3.max(allNodes, d => d.x) || width
    const treeW = maxX - minX
    const treeH = d3.max(allNodes, d => d.y) || height

    const fitScale = Math.min(1, (width - 80) / (treeW + 100), (height - 80) / (treeH + 100))
    const centerX = -(minX + maxX) / 2

    const linkGroup = g.append('g').attr('class', 'links')
    const nodeGroup = g.append('g').attr('class', 'nodes')

    linkGroup.selectAll('.link')
      .data(allLinks)
      .enter()
      .append('line')
      .attr('class', 'link')
      .attr('x1', d => d.source.x)
      .attr('y1', d => d.source.y + getNodeRadius(d.source))
      .attr('x2', d => d.target.x)
      .attr('y2', d => d.target.y - getNodeRadius(d.target))
      .attr('stroke', '#4a90e2')
      .attr('stroke-width', 2)
      .attr('opacity', 0.6)

    const nodeSel = nodeGroup.selectAll('.node')
      .data(allNodes)
      .enter()
      .append('g')
      .attr('class', 'node')
      .attr('transform', d => `translate(${d.x},${d.y})`)

    nodeSel.append('circle')
      .attr('r', d => getNodeRadius(d))
      .attr('fill', d => {
        if (d.data.type?.includes('Stm')) return '#e74c3c'
        if (d.data.type?.includes('Exp')) return '#3498db'
        if (d.data.type?.includes('Lit')) return '#2ecc71'
        return '#95a5a6'
      })
      .attr('stroke', '#ecf0f1')
      .attr('stroke-width', 1.5)

    nodeSel.append('text')
      .attr('text-anchor', 'middle')
      .attr('fill', 'white')
      .attr('font-weight', 'bold')
      .attr('pointer-events', 'none')
      .each(function(d) {
        const label = getNodeLabel(d)
        const pad = 6
        const availW = NODE_RADIUS * 2 - pad * 2
        let fontSize = 10
        for (; fontSize >= 4; fontSize -= 0.5) {
          if (label.length * fontSize * 0.6 <= availW) break
        }
        d3.select(this)
          .attr('font-size', fontSize + 'px')
          .attr('dy', '0.35em')
          .text(label)
      })

    const zoom_handler = d3.zoom()
      .scaleExtent([0.1, 4])
      .on('zoom', (event) => {
        g.attr('transform', event.transform)
        setZoom(event.transform.k)
      })

    svg.call(zoom_handler)
    svg.call(zoom_handler.transform, d3.zoomIdentity.translate(width / 2 + centerX * fitScale, 40).scale(fitScale))

  }, [ast])

  if (loading) {
    return (
      <div style={{
        width: '100%', height: '100%',
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        color: '#999',
      }}>
        <div>Compilando...</div>
      </div>
    )
  }

  if (!ast) {
    return (
      <div style={{
        width: '100%', height: '100%',
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        color: '#666', flexDirection: 'column',
      }}>
        <div style={{ fontSize: '48px', marginBottom: '16px' }}>{'>'}</div>
        <div>Compila codigo para ver el AST</div>
      </div>
    )
  }

  return (
    <div
      ref={containerRef}
      style={{
        width: '100%', height: '100%', position: 'relative',
        background: '#1e1e1e', overflow: 'hidden',
      }}
    >
      <svg
        ref={svgRef}
        style={{
          width: '100%', height: '100%', background: '#252526',
        }}
      />
      <div style={{
        position: 'absolute', bottom: '12px', right: '12px',
        fontSize: '12px', color: '#666',
        background: 'rgba(0, 0, 0, 0.3)',
        padding: '6px 12px', borderRadius: '4px',
      }}>
        Zoom: {(zoom * 100).toFixed(0)}%
      </div>
    </div>
  )
}
