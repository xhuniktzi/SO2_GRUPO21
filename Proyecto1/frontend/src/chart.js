import React from 'react';
import { Pie } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  ArcElement,
  Tooltip,
  Legend
} from 'chart.js';

ChartJS.register(ArcElement, Tooltip, Legend);

const PieChart = ({ data }) => {
  const processData = () => {
    const processMemory = {};

    data.forEach(log => {
      if (processMemory[log.process_name]) {
        processMemory[log.process_name] += log.memory_size_mb;
      } else {
        processMemory[log.process_name] = log.memory_size_mb;
      }
    });

    // Sort processes by memory usage
    const sortedProcesses = Object.entries(processMemory).sort((a, b) => b[1] - a[1]);

    // Limit to top 9 processes and group the rest into "otros"
    const topProcesses = sortedProcesses.slice(0, 10);
    const otherProcesses = sortedProcesses.slice(10);

    const otherMemory = otherProcesses.reduce((acc, curr) => acc + curr[1], 0);

    const labels = [...topProcesses.map(process => process[0]), 'otros'];
    const memorySizes = [...topProcesses.map(process => process[1]), otherMemory];

    return {
      labels: labels,
      datasets: [
        {
          data: memorySizes,
          backgroundColor: [
            '#FF6384',
            '#36A2EB',
            '#FFCE56',
            '#4BC0C0',
            '#9966FF',
            '#FF9F40',
            '#FF6384',
            '#36A2EB',
            '#FFCE56',
            '#4BC0C0',
            '#9966FF'
          ],
          hoverBackgroundColor: [
            '#FF6384',
            '#36A2EB',
            '#FFCE56',
            '#4BC0C0',
            '#9966FF',
            '#FF9F40',
            '#FF6384',
            '#36A2EB',
            '#FFCE56',
            '#4BC0C0',
            '#9966FF'
          ]
        }
      ]
    };
  };

  const options = {
    plugins: {
      legend: {
        display: true,
        position: 'right'
      }
    }
  };

  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', justifyContent: 'center', alignItems: 'center' }}>
      <div style={{ width: '100%', height: '100%',  justifyContent: 'center', alignItems: 'center' }}>
        <Pie data={processData()} options={options} />
      </div>
    </div>
  );
};

export default PieChart;
