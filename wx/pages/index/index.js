Page({
  data: {
    temperature: '--',
    humidity: '--',
    light: '--',
    soil: '--',
    online: false
  },

  onLoad() {
    // 模拟数据，测试UI效果
    this.setData({
      temperature: 26,
      humidity: 45,
      light: 320,
      soil: 60,
      online: true
    });
  },

  // 跳转到控制页面
  goToControl() {
    wx.navigateTo({
      url: '/pages/control/control'  // 跳转到控制页面
    });
  },

  // 跳转到设置页面
  goToSettings() {
    wx.navigateTo({
      url: '/pages/settings/settings'  // 跳转到设置页面
    });
  }
});
