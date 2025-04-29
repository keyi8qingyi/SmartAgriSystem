import * as echarts from '../../ec-canvas/echarts'

Page({
  data: {
    ec: {
      lazyLoad: true
    }
  },

  onReady() {
    // 延迟执行，确保组件已加载
    setTimeout(() => {
      this.initChart();
    }, 1000);  // 延迟 1 秒初始化
  },

  initChart() {
    // 查找图表组件
    this.ecComponent = this.selectComponent('#mychart');

    // 确保找到组件实例
    if (this.ecComponent) {
      this.ecComponent.init((canvas, width, height, dpr) => {
        const chart = echarts.init(canvas, null, {
          width: width,
          height: height,
          devicePixelRatio: dpr
        });
        canvas.setChart(chart);

        const option = {
          title: {
            text: '历史数据'
          },
          xAxis: {
            type: 'category',
            data: ['2025-04-01', '2025-04-02', '2025-04-03', '2025-04-04', '2025-04-05']
          },
          yAxis: {
            type: 'value'
          },
          series: [{
            data: [820, 932, 901, 934, 1290],
            type: 'line'
          }]
        };

        chart.setOption(option);
        return chart;
      });
    } else {
      console.error('未找到 ec-canvas 组件');
    }
  },

  // 返回首页
  goToHome() {
    wx.navigateBack();  // 返回上一页，即首页
  }
});
