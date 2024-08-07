import React, { useState, useEffect } from 'react';
import axios from 'axios';
import PieChart from './chart.js';
import './log.css';

const MemoryLogs = () => {
    const [individualLogs, setIndividualLogs] = useState([]);
    const [aggregatedLogs, setAggregatedLogs] = useState([]);
  
    const fetchLogs = () => {
      axios.get('http://localhost:8080/api/memorylogs')
        .then(response => {
          const data = response.data;
          setIndividualLogs(data.individual_logs);
          setAggregatedLogs(data.aggregated_logs);
        })
        .catch(error => {
          console.error('Error fetching data:', error);
        });
    };
  
    useEffect(() => {
      fetchLogs(); 
      const interval = setInterval(fetchLogs, 15000); 
  
      return () => clearInterval(interval); 
    }, []);
  

  return (
    <div className="dashboard">
      <header className="App-header">
        <h1>Dashboard</h1>
      </header>
      <div className="top-half">
        <div className="chart">
          <PieChart data={aggregatedLogs} />
        </div>
        <div className="table1">
          <h2>Resumen de Memoria por Proceso</h2>
          <table>
            <thead>
              <tr>
                <th>PID</th>
                <th>Nombre</th>
                <th>Memoria (MB)</th>
                <th>Memoria (%)</th>
              </tr>
            </thead>
            <tbody>
              {aggregatedLogs.map(log => (
                <tr key={log.pid + log.timestamp}>
                  <td>{log.pid}</td>
                  <td>{log.process_name}</td>
                  <td>{log.total_memory_mb} MB</td>
                  <td>{log.total_memory_per}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
      <div className="bottom-half">
        <h2>Historial de Solicitudes</h2>
        <table>
          <thead>
            <tr>
              <th>PID</th>
              <th>Llamada</th>
              <th>Tamaño</th>
              <th>Fecha</th>
            </tr>
          </thead>
          <tbody>
            {individualLogs.map(log => (
              <tr key={log.pid + log.timestamp}>
                <td>{log.pid}</td>
                <td>{log.call_type}</td>
                <td>{log.memory_size_mb} MB</td>
                <td>{new Date(log.timestamp).toLocaleString()}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
};


export default MemoryLogs;
