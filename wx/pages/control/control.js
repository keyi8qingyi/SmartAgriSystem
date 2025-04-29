Page({
  data: {
    irrigationStatus: false,
    threshold: 60,
    lightThreshold: 300 // 光照阈值的默认值
  },

  toggleIrrigation() {
    this.setData({
      irrigationStatus: !this.data.irrigationStatus
    });
  },

  updateThreshold(e) {
    this.setData({
      threshold: e.detail.value
    });
  },

  updateLightThreshold(e) {
    this.setData({
      lightThreshold: e.detail.value
    });
  },

  // 返回首页
  goToHome() {
    wx.navigateBack();  // 返回上一页，即首页
  }
});
